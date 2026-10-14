//! The lobby's line protocol. One message per line, fields separated by tabs, UTF-8, at most
//! `MAX_LINE` bytes. Everything a client sends is checked here before the lobby acts on it.
//!
//! ```text
//! client                                         server
//! HELLO <protocol> <build> <player name>         OK <nonce>            | ERROR <reason>
//! HOST <game name> <salt|-> <verifier|-> [<character>]
//!                                                HOSTING <id>          | ERROR <reason>
//! PING                                           PONG
//! LIST                                           GAME <id> <name> <locked 0|1> <host> <age s> <salt|-> <character|->  ...  END
//! JOIN <id> <proof|->                            OK                    | DENIED <reason>
//! (host, when someone has joined)                JOINED <player name>
//! READY                                          (both connections now carry the game's own bytes)
//! ```
//!
//! Passwords: the host sends `verifier = sha256(salt + password)`; a joiner proves knowledge with
//! `proof = sha256(nonce + verifier)`, where the nonce is the one the server gave that joiner in OK. The
//! password itself never crosses the network and the server never sees it.
//!
//! `character`: the host's character as the game shows it in a save slot (name, level, class, looks), as
//! lower-case hex of at most `MAX_CHARACTER` characters. The server passes it on without reading it. It is
//! optional: a game that does not send it can still host, and one that does not expect it ignores the last
//! field of GAME.

pub const PROTOCOL: u32 = 1;
pub const MAX_LINE: usize = 1024;
pub const MAX_CHARACTER: usize = 480;
pub const MAX_NAME: usize = 32;

#[derive(Debug, PartialEq, Eq)]
pub enum Request {
    Hello { protocol: u32, build: String, player: String },
    Host { name: String, salt: Option<String>, verifier: Option<String>, character: Option<String> },
    Ping,
    List,
    Join { id: u32, proof: Option<String> },
    Ready,
}

/// A name a person typed: 1 to `MAX_NAME` characters, nothing that could break a line or a terminal.
pub fn valid_name(text: &str) -> bool {
    let length = text.chars().count();
    (1..=MAX_NAME).contains(&length) && text.trim() == text && text.chars().all(|c| !c.is_control() && c != '\t')
}

/// Lower-case hexadecimal of exactly `length` characters.
pub fn valid_hex(text: &str, length: usize) -> bool {
    text.len() == length && text.bytes().all(|b| b.is_ascii_digit() || (b'a'..=b'f').contains(&b))
}

fn optional_hex(field: &str, length: usize) -> Result<Option<String>, &'static str> {
    match field {
        "-" => Ok(None),
        text if valid_hex(text, length) => Ok(Some(text.to_string())),
        _ => Err("malformed field"),
    }
}

pub fn parse(line: &str) -> Result<Request, &'static str> {
    if line.len() > MAX_LINE {
        return Err("line too long");
    }
    let fields: Vec<&str> = line.split('\t').collect();
    match fields.as_slice() {
        ["HELLO", protocol, build, player] => {
            let protocol = protocol.parse().map_err(|_| "malformed protocol number")?;
            if !valid_hex(build, 16) {
                return Err("malformed build id");
            }
            if !valid_name(player) {
                return Err("player name not acceptable");
            }
            Ok(Request::Hello { protocol, build: build.to_string(), player: player.to_string() })
        }
        ["HOST", name, salt, verifier, rest @ ..] if rest.len() <= 1 => {
            if !valid_name(name) {
                return Err("game name not acceptable");
            }
            let salt = optional_hex(salt, 16)?;
            let verifier = optional_hex(verifier, 64)?;
            if salt.is_some() != verifier.is_some() {
                return Err("salt and verifier go together");
            }
            let character = match rest.first() {
                None | Some(&"-") => None,
                Some(text) if text.len() % 2 == 0 && text.len() <= MAX_CHARACTER && valid_hex(text, text.len()) => Some(text.to_string()),
                Some(_) => return Err("malformed character"),
            };
            Ok(Request::Host { name: name.to_string(), salt, verifier, character })
        }
        ["PING"] => Ok(Request::Ping),
        ["LIST"] => Ok(Request::List),
        ["JOIN", id, proof] => Ok(Request::Join {
            id: id.parse().map_err(|_| "malformed game id")?,
            proof: optional_hex(proof, 64)?,
        }),
        ["READY"] => Ok(Request::Ready),
        _ => Err("unknown request"),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn accepts_the_documented_requests() {
        assert_eq!(
            parse("HELLO\t1\t0123456789abcdef\tPlayer"),
            Ok(Request::Hello { protocol: 1, build: "0123456789abcdef".into(), player: "Player".into() })
        );
        assert_eq!(parse("HOST\tMy game\t-\t-"), Ok(Request::Host { name: "My game".into(), salt: None, verifier: None, character: None }));
        assert_eq!(parse("HOST\tMy game\t-\t-\t-"), Ok(Request::Host { name: "My game".into(), salt: None, verifier: None, character: None }));
        assert_eq!(
            parse("HOST\tMy game\t-\t-\t0aff"),
            Ok(Request::Host { name: "My game".into(), salt: None, verifier: None, character: Some("0aff".into()) })
        );
        let verifier = "a".repeat(64);
        assert_eq!(
            parse(&format!("HOST\tLocked\t00112233aabbccdd\t{verifier}")),
            Ok(Request::Host { name: "Locked".into(), salt: Some("00112233aabbccdd".into()), verifier: Some(verifier), character: None })
        );
        assert_eq!(parse("JOIN\t7\t-"), Ok(Request::Join { id: 7, proof: None }));
        assert_eq!(parse("LIST"), Ok(Request::List));
        assert_eq!(parse("PING"), Ok(Request::Ping));
        assert_eq!(parse("READY"), Ok(Request::Ready));
    }

    #[test]
    fn rejects_bad_input() {
        assert!(parse("").is_err());
        assert!(parse("HOST\t\t-\t-").is_err());
        assert!(parse("HOST\t leading space\t-\t-").is_err());
        assert!(parse(&format!("HOST\t{}\t-\t-", "x".repeat(33))).is_err());
        assert!(parse("HOST\tname\t00112233aabbccdd\t-").is_err());
        assert!(parse("HOST\tname\tXYZ\tXYZ").is_err());
        assert!(parse("HOST\tbell\u{7}\t-\t-").is_err());
        assert!(parse("HOST\tname\t-\t-\tABCD").is_err());
        assert!(parse("HOST\tname\t-\t-\t0").is_err());
        assert!(parse(&format!("HOST\tname\t-\t-\t{}", "0".repeat(482))).is_err());
        assert!(parse("HOST\tname\t-\t-\t00\textra").is_err());
        assert!(parse("JOIN\tseven\t-").is_err());
        assert!(parse("JOIN\t7\tABCDEF").is_err());
        assert!(parse("HELLO\t1\tshort\tPlayer").is_err());
        assert!(parse(&"L".repeat(1100)).is_err());
        assert!(parse("DROP TABLE").is_err());
    }
}
