//! The lobby: who is hosting, who may join, and passing a matched pair's bytes between them.
//!
//! One thread per connection. A hosting connection waits in `wait_for_joiner`; a joining connection that
//! is accepted hands its socket to that host through the game's `Match` slot, and from then on each of the
//! two threads copies its own client's bytes to the other client, without looking at them.

use crate::protocol::{self, Request};
use crate::sha256::hex_digest;
use std::collections::hash_map::RandomState;
use std::collections::HashMap;
use std::hash::{BuildHasher, Hasher};
use std::io::{self, BufRead, BufReader, Read, Write};
use std::net::{IpAddr, Shutdown, TcpListener, TcpStream};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::mpsc::{self, Receiver, Sender};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::{Duration, Instant};

#[derive(Clone, Debug)]
pub struct Limits {
    pub max_games: usize,
    pub max_connections_per_address: usize,
    /// A client must say something this often while in the lobby (hosts send PING).
    pub lobby_idle: Duration,
    /// A matched pair that exchanges nothing for this long is dropped.
    pub relay_idle: Duration,
}

impl Default for Limits {
    fn default() -> Self {
        Limits {
            max_games: 200,
            max_connections_per_address: 8,
            lobby_idle: Duration::from_secs(90),
            relay_idle: Duration::from_secs(120),
        }
    }
}

/// What a joiner hands to the host it was accepted by.
struct Joiner {
    stream: TcpStream,
    player: String,
    /// The host's thread sends its own socket here once the host's client is ready for the game's bytes;
    /// if the host fails first, this end is dropped and the joiner is told so.
    host: Sender<TcpStream>,
}

struct Game {
    name: String,
    host: String,
    build: String,
    salt: Option<String>,
    verifier: Option<String>,
    /// The host's character, as the game sends it (see protocol.rs); passed on unread.
    character: Option<String>,
    since: Instant,
    /// Where an accepted joiner is put for the host's thread to find.
    matched: Sender<Joiner>,
}

#[derive(Default)]
struct State {
    games: HashMap<u32, Game>,
    connections: HashMap<IpAddr, usize>,
}

pub struct Lobby {
    state: Mutex<State>,
    next_id: AtomicU32,
    limits: Limits,
}

fn nonce() -> String {
    // Two hashes with fresh random keys from the operating system (that is what RandomState is).
    let a = RandomState::new().build_hasher().finish();
    let b = RandomState::new().build_hasher().finish();
    format!("{a:016x}{b:016x}")
}

fn read_line(reader: &mut BufReader<TcpStream>) -> io::Result<Option<String>> {
    let mut bytes = Vec::new();
    let read = reader.by_ref().take(protocol::MAX_LINE as u64 + 1).read_until(b'\n', &mut bytes)?;
    if read == 0 {
        return Ok(None);
    }
    if bytes.last() != Some(&b'\n') {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "line too long or cut off"));
    }
    bytes.pop();
    if bytes.last() == Some(&b'\r') {
        bytes.pop();
    }
    String::from_utf8(bytes).map(Some).map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "not UTF-8"))
}

fn send(stream: &mut TcpStream, line: &str) -> io::Result<()> {
    stream.write_all(line.as_bytes())?;
    stream.write_all(b"\n")
}

/// Copies one client's bytes to the other until either side ends. Never interprets them.
fn relay(from: &mut BufReader<TcpStream>, to: &mut TcpStream, idle: Duration) -> u64 {
    let _ = from.get_ref().set_read_timeout(Some(idle));
    let mut total = 0u64;
    let mut buffer = [0u8; 16 * 1024];
    loop {
        match from.read(&mut buffer) {
            Ok(0) | Err(_) => break,
            Ok(count) => {
                if to.write_all(&buffer[..count]).is_err() {
                    break;
                }
                total += count as u64;
            }
        }
    }
    let _ = to.shutdown(Shutdown::Both);
    let _ = from.get_ref().shutdown(Shutdown::Both);
    total
}

impl Lobby {
    pub fn new(limits: Limits) -> Arc<Lobby> {
        Arc::new(Lobby { state: Mutex::new(State::default()), next_id: AtomicU32::new(1), limits })
    }

    pub fn games(&self) -> usize {
        self.state.lock().unwrap().games.len()
    }

    /// Accepts connections for ever, one thread each.
    pub fn serve(self: &Arc<Self>, listener: TcpListener) {
        for stream in listener.incoming().flatten() {
            let lobby = Arc::clone(self);
            thread::spawn(move || lobby.connection(stream));
        }
    }

    fn connection(self: Arc<Self>, stream: TcpStream) {
        let Ok(address) = stream.peer_addr().map(|a| a.ip()) else { return };
        {
            let mut state = self.state.lock().unwrap();
            let count = state.connections.entry(address).or_insert(0);
            if *count >= self.limits.max_connections_per_address {
                drop(state);
                let mut stream = stream;
                let _ = send(&mut stream, "ERROR\ttoo many connections from your address");
                return;
            }
            *count += 1;
        }
        let outcome = self.talk(stream, address);
        let mut state = self.state.lock().unwrap();
        if let Some(count) = state.connections.get_mut(&address) {
            *count -= 1;
            if *count == 0 {
                state.connections.remove(&address);
            }
        }
        drop(state);
        if let Err(error) = outcome {
            println!("{address}: closed: {error}");
        }
    }

    fn talk(&self, stream: TcpStream, address: IpAddr) -> io::Result<()> {
        stream.set_nodelay(true)?;
        stream.set_read_timeout(Some(self.limits.lobby_idle))?;
        stream.set_write_timeout(Some(Duration::from_secs(20)))?;
        let mut out = stream.try_clone()?;
        let mut reader = BufReader::new(stream);

        // the first line must be HELLO
        let (build, player) = match read_line(&mut reader)?.as_deref().map(protocol::parse) {
            Some(Ok(Request::Hello { protocol: protocol::PROTOCOL, build, player })) => (build, player),
            Some(Ok(Request::Hello { .. })) => return send(&mut out, "ERROR\tthis server speaks a different protocol version"),
            Some(Err(reason)) => return send(&mut out, &format!("ERROR\t{reason}")),
            _ => return send(&mut out, "ERROR\tsay HELLO first"),
        };
        println!("{address}: {player} says hello, build {build}");
        let nonce = nonce();
        send(&mut out, &format!("OK\t{nonce}"))?;

        while let Some(line) = read_line(&mut reader)? {
            match protocol::parse(&line) {
                Err(reason) => send(&mut out, &format!("ERROR\t{reason}"))?,
                Ok(Request::Ping) => send(&mut out, "PONG")?,
                Ok(Request::List) => {
                    let (lines, others) = {
                        let state = self.state.lock().unwrap();
                        // games of other builds are not offered, only counted, so the player learns why the list is short
                        let others = state.games.values().filter(|g| g.build != build).count();
                        let mut games: Vec<(&u32, &Game)> = state.games.iter().filter(|(_, g)| g.build == build).collect();
                        games.sort_by_key(|(id, _)| **id);
                        let lines = games
                            .iter()
                            .map(|(id, g)| {
                                format!(
                                    "GAME\t{id}\t{}\t{}\t{}\t{}\t{}\t{}",
                                    g.name,
                                    u8::from(g.verifier.is_some()),
                                    g.host,
                                    g.since.elapsed().as_secs(),
                                    g.salt.as_deref().unwrap_or("-"),
                                    g.character.as_deref().unwrap_or("-")
                                )
                            })
                            .collect::<Vec<String>>();
                        (lines, others)
                    };
                    for line in lines {
                        send(&mut out, &line)?;
                    }
                    if others > 0 {
                        send(&mut out, &format!("OTHER\t{others}"))?;
                    }
                    send(&mut out, "END")?;
                }
                Ok(Request::Host { name, salt, verifier, character }) => {
                    let (matched, waiting) = mpsc::channel();
                    let id = self.next_id.fetch_add(1, Ordering::Relaxed);
                    {
                        let mut state = self.state.lock().unwrap();
                        if state.games.len() >= self.limits.max_games {
                            drop(state);
                            send(&mut out, "ERROR\tthe server is full")?;
                            continue;
                        }
                        state.games.insert(
                            id,
                            Game {
                                name: name.clone(),
                                host: player.clone(),
                                build: build.clone(),
                                salt,
                                verifier,
                                character,
                                since: Instant::now(),
                                matched,
                            },
                        );
                    }
                    println!("{address}: {player} hosts game {id} \"{name}\"");
                    send(&mut out, &format!("HOSTING\t{id}"))?;
                    let result = self.wait_for_joiner(&mut reader, &mut out, waiting);
                    self.state.lock().unwrap().games.remove(&id);
                    println!("{address}: game {id} ended");
                    return result;
                }
                Ok(Request::Join { id, proof }) => {
                    // the game leaves the list the moment someone is accepted: one joiner per game
                    let accepted = {
                        let mut state = self.state.lock().unwrap();
                        match state.games.get(&id) {
                            None => Err("that game is no longer there"),
                            Some(game) if game.build != build => Err("that game runs a different build"),
                            Some(game) => match (&game.verifier, &proof) {
                                (None, _) => Ok(()),
                                (Some(_), None) => Err("this game needs its password"),
                                (Some(verifier), Some(proof)) => {
                                    if hex_digest(format!("{nonce}{verifier}").as_bytes()) == *proof {
                                        Ok(())
                                    } else {
                                        Err("wrong password")
                                    }
                                }
                            },
                        }
                        .map(|()| state.games.remove(&id).unwrap())
                    };
                    let game = match accepted {
                        Ok(game) => game,
                        Err(reason) => {
                            thread::sleep(Duration::from_millis(400));     // slows password guessing a little
                            send(&mut out, &format!("DENIED\t{reason}"))?;
                            continue;
                        }
                    };
                    let (host, host_stream) = mpsc::channel();
                    if game.matched.send(Joiner { stream: out.try_clone()?, player: player.clone(), host }).is_err() {
                        send(&mut out, "DENIED\tthe host has just left")?;
                        continue;
                    }
                    let Ok(mut host_stream) = host_stream.recv_timeout(Duration::from_secs(40)) else {
                        send(&mut out, "DENIED\tthe host did not answer")?;
                        return Ok(());
                    };
                    println!("{address}: {player} joined game {id} \"{}\"", game.name);
                    send(&mut out, "OK")?;
                    let bytes = relay(&mut reader, &mut host_stream, self.limits.relay_idle);
                    println!("{address}: game {id}: {bytes} bytes from the joiner");
                    return Ok(());
                }
                Ok(Request::Hello { .. }) | Ok(Request::Ready) => send(&mut out, "ERROR\tnot now")?,
            }
        }
        Ok(())
    }

    /// The host's side after HOSTING: answer PINGs until a joiner arrives or the host goes away, then
    /// tell the host, wait for its READY, and relay.
    fn wait_for_joiner(&self, reader: &mut BufReader<TcpStream>, out: &mut TcpStream, waiting: Receiver<Joiner>) -> io::Result<()> {
        reader.get_ref().set_read_timeout(Some(Duration::from_millis(250)))?;
        let mut last_heard = Instant::now();
        let joiner = loop {
            if let Ok(joiner) = waiting.try_recv() {
                break joiner;
            }
            match read_line(reader) {
                Ok(None) => return Ok(()),
                Ok(Some(line)) => {
                    last_heard = Instant::now();
                    match protocol::parse(&line) {
                        Ok(Request::Ping) => send(out, "PONG")?,
                        _ => send(out, "ERROR\tonly PING while hosting")?,
                    }
                }
                Err(error) if matches!(error.kind(), io::ErrorKind::WouldBlock | io::ErrorKind::TimedOut) => {
                    if last_heard.elapsed() > self.limits.lobby_idle {
                        return Err(io::Error::new(io::ErrorKind::TimedOut, "host went quiet"));
                    }
                }
                Err(error) => return Err(error),
            }
        };
        let mut joiner_stream = joiner.stream;
        send(out, &format!("JOINED\t{}", joiner.player))?;
        // the host's client may still have a PING on the way; READY marks the end of its lobby talk
        reader.get_ref().set_read_timeout(Some(Duration::from_secs(20)))?;
        loop {
            match read_line(reader)?.as_deref().map(protocol::parse) {
                Some(Ok(Request::Ready)) => break,
                Some(Ok(Request::Ping)) => {}
                _ => return Err(io::Error::new(io::ErrorKind::InvalidData, "host did not say READY")),
            }
        }
        if joiner.host.send(out.try_clone()?).is_err() {
            return Ok(());                                      // the joiner gave up meanwhile
        }
        let bytes = relay(reader, &mut joiner_stream, self.limits.relay_idle);
        println!("relay: {bytes} bytes from the host");
        Ok(())
    }
}
