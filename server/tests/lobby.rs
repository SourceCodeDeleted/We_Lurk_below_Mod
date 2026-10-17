//! The whole server, exercised through real sockets on a port of its own.
use ilb_lobby::lobby::{Limits, Lobby};
use ilb_lobby::sha256::hex_digest;
use std::io::{BufRead, BufReader, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

const BUILD: &str = "0123456789abcdef";

fn start(limits: Limits) -> (Arc<Lobby>, u16) {
    let listener = TcpListener::bind("127.0.0.1:0").unwrap();
    let port = listener.local_addr().unwrap().port();
    let lobby = Lobby::new(limits);
    let serving = Arc::clone(&lobby);
    thread::spawn(move || serving.serve(listener));
    (lobby, port)
}

struct Client {
    reader: BufReader<TcpStream>,
    nonce: String,
}

impl Client {
    fn raw(port: u16) -> Client {
        let stream = TcpStream::connect(("127.0.0.1", port)).unwrap();
        stream.set_read_timeout(Some(Duration::from_secs(10))).unwrap();
        Client { reader: BufReader::new(stream), nonce: String::new() }
    }
    fn connect(port: u16, player: &str, build: &str) -> Client {
        let mut client = Client::raw(port);
        let answer = client.ask(&format!("HELLO\t1\t{build}\t{player}"));
        client.nonce = answer.strip_prefix("OK\t").expect(&answer).to_string();
        client
    }
    fn say(&mut self, line: &str) {
        self.reader.get_mut().write_all(format!("{line}\n").as_bytes()).unwrap();
    }
    fn hear(&mut self) -> String {
        let mut line = String::new();
        self.reader.read_line(&mut line).unwrap();
        line.trim_end().to_string()
    }
    fn ask(&mut self, line: &str) -> String {
        self.say(line);
        self.hear()
    }
    fn list(&mut self) -> Vec<String> {
        self.say("LIST");
        let mut games = Vec::new();
        loop {
            let line = self.hear();
            if line == "END" {
                return games;
            }
            games.push(line);
        }
    }
}

fn verifier(salt: &str, password: &str) -> String {
    hex_digest(format!("{salt}{password}").as_bytes())
}

#[test]
fn the_hosts_character_is_passed_on_with_the_list() {
    let (_lobby, port) = start(Limits::default());
    let mut host = Client::connect(port, "Host", BUILD);
    let mut joiner = Client::connect(port, "Guest", BUILD);
    let character = "0a1b".repeat(102);                 // 408 hex characters, the size the game sends
    assert!(host.ask(&format!("HOST\tWith hero\t-\t-\t{character}")).starts_with("HOSTING\t"));
    let games = joiner.list();
    assert_eq!(games.len(), 1);
    assert!(games[0].ends_with(&format!("\t-\t{character}")), "{}", games[0]);
}

#[test]
fn open_game_is_listed_joined_and_relayed_both_ways() {
    let (lobby, port) = start(Limits::default());
    let mut host = Client::connect(port, "Host", BUILD);
    let mut joiner = Client::connect(port, "Guest", BUILD);
    assert!(joiner.list().is_empty());

    assert_eq!(host.ask("HOST\tFriday night\t-\t-"), "HOSTING\t1");
    assert_eq!(host.ask("PING"), "PONG");
    let games = joiner.list();
    assert_eq!(games.len(), 1);
    assert!(games[0].starts_with("GAME\t1\tFriday night\t0\tHost\t"), "{}", games[0]);
    assert!(games[0].ends_with("\t-\t-"), "no password, no character: {}", games[0]);

    joiner.say("JOIN\t1\t-");
    assert_eq!(host.hear(), "JOINED\tGuest");
    host.say("PING");                                   // one that was already on its way: must not reach the joiner
    host.say("READY");
    assert_eq!(joiner.hear(), "OK");
    assert_eq!(lobby.games(), 0, "a matched game leaves the list");

    // from here on the server only passes bytes along, in both directions, unchanged
    host.reader.get_mut().write_all(b"\x00\x01binary from host\n\xff").unwrap();
    joiner.reader.get_mut().write_all(b"and back").unwrap();
    let mut from_host = [0u8; 20];
    joiner.reader.read_exact(&mut from_host).unwrap();
    assert_eq!(&from_host, b"\x00\x01binary from host\n\xff");
    let mut from_joiner = [0u8; 8];
    host.reader.read_exact(&mut from_joiner).unwrap();
    assert_eq!(&from_joiner, b"and back");

    // one side leaving ends it for the other
    drop(host);
    let mut rest = Vec::new();
    assert_eq!(joiner.reader.read_to_end(&mut rest).unwrap_or(0), 0);
}

#[test]
fn password_is_checked_without_being_sent() {
    let (_lobby, port) = start(Limits::default());
    let salt = "00112233aabbccdd";
    let mut host = Client::connect(port, "Host", BUILD);
    assert!(host.ask(&format!("HOST\tPrivate\t{salt}\t{}", verifier(salt, "sesame"))).starts_with("HOSTING\t"));

    let mut joiner = Client::connect(port, "Guest", BUILD);
    let games = joiner.list();
    let fields: Vec<&str> = games[0].split('\t').collect();
    assert_eq!((fields[3], fields[6]), ("1", salt), "locked, and the salt is published so a joiner can answer");
    let id = fields[1];

    assert_eq!(joiner.ask(&format!("JOIN\t{id}\t-")), "DENIED\tthis game needs its password");
    let wrong = hex_digest(format!("{}{}", joiner.nonce, verifier(salt, "guess")).as_bytes());
    assert_eq!(joiner.ask(&format!("JOIN\t{id}\t{wrong}")), "DENIED\twrong password");
    // a proof made for another connection's nonce is useless here
    let mut other = Client::connect(port, "Eve", BUILD);
    let stolen = hex_digest(format!("{}{}", joiner.nonce, verifier(salt, "sesame")).as_bytes());
    assert_eq!(other.ask(&format!("JOIN\t{id}\t{stolen}")), "DENIED\twrong password");

    let right = hex_digest(format!("{}{}", joiner.nonce, verifier(salt, "sesame")).as_bytes());
    joiner.say(&format!("JOIN\t{id}\t{right}"));
    assert_eq!(host.hear(), "JOINED\tGuest");
    host.say("READY");
    assert_eq!(joiner.hear(), "OK");
}

#[test]
fn games_are_only_shown_to_and_joinable_by_the_same_build() {
    let (_lobby, port) = start(Limits::default());
    let mut host = Client::connect(port, "Host", BUILD);
    host.ask("HOST\tOld build\t-\t-");
    let mut other = Client::connect(port, "Guest", "ffffffffffffffff");
    assert_eq!(other.list(), vec!["OTHER\t1".to_string()]);
    assert_eq!(other.ask("JOIN\t1\t-"), "DENIED\tthat game runs a different build");
}

#[test]
fn a_game_disappears_when_its_host_leaves() {
    let (lobby, port) = start(Limits::default());
    let mut host = Client::connect(port, "Host", BUILD);
    host.ask("HOST\tShort lived\t-\t-");
    assert_eq!(lobby.games(), 1);
    drop(host);
    for _ in 0..50 {
        if lobby.games() == 0 {
            break;
        }
        thread::sleep(Duration::from_millis(100));
    }
    assert_eq!(lobby.games(), 0);
    let mut joiner = Client::connect(port, "Guest", BUILD);
    assert_eq!(joiner.ask("JOIN\t1\t-"), "DENIED\tthat game is no longer there");
}

#[test]
fn limits_and_bad_input_are_refused_politely() {
    let limits = Limits { max_games: 1, max_connections_per_address: 3, ..Limits::default() };
    let (_lobby, port) = start(limits);
    let mut first = Client::connect(port, "One", BUILD);
    assert!(first.ask("HOST\tOnly one\t-\t-").starts_with("HOSTING"));
    let mut second = Client::connect(port, "Two", BUILD);
    assert_eq!(second.ask("HOST\tOne too many\t-\t-"), "ERROR\tthe server is full");
    assert_eq!(second.ask("HOST\t\t-\t-"), "ERROR\tgame name not acceptable");
    assert_eq!(second.ask("NONSENSE"), "ERROR\tunknown request");
    assert_eq!(second.ask("HELLO\t1\t0123456789abcdef\tAgain"), "ERROR\tnot now");

    let _third = Client::connect(port, "Three", BUILD);
    let mut fourth = Client::raw(port);
    assert_eq!(fourth.hear(), "ERROR\ttoo many connections from your address");

    drop(second);
    thread::sleep(Duration::from_millis(300));
    let mut stranger = Client::raw(port);
    assert_eq!(stranger.ask("LIST"), "ERROR\tsay HELLO first");
    let mut old = Client::raw(port);
    assert_eq!(old.ask("HELLO\t99\t0123456789abcdef\tOld"), "ERROR\tthis server speaks a different protocol version");
}
