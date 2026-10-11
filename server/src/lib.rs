//! Lobby and relay server for two-player games of the It Lurks Below rebuild.
//!
//! A game copy connects, says who it is, and either hosts a game (optionally with a password) or lists
//! the games and joins one. Once a joiner is accepted the server stops reading what the two say and only
//! passes their bytes to each other, so two players behind home routers can play without opening ports.
pub mod lobby;
pub mod protocol;
pub mod sha256;
