use ilb_lobby::lobby::{Limits, Lobby};
use std::net::TcpListener;

fn main() {
    let mut port = 27960u16;
    let mut arguments = std::env::args().skip(1);
    while let Some(argument) = arguments.next() {
        match argument.as_str() {
            "--port" => match arguments.next().and_then(|value| value.parse().ok()) {
                Some(value) => port = value,
                None => return eprintln!("--port needs a number"),
            },
            _ => return eprintln!("usage: ilb-lobby [--port <number>]      (default 27960)"),
        }
    }
    let listener = match TcpListener::bind(("0.0.0.0", port)) {
        Ok(listener) => listener,
        Err(error) => return eprintln!("cannot listen on port {port}: {error}"),
    };
    println!("ilb-lobby: listening on port {port}");
    Lobby::new(Limits::default()).serve(listener);
}
