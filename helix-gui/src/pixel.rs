//! 1-bit mac pixel assets: kare-style 16x16 maps rendered into egui
//! textures (nearest, no icon fonts, no external files). three tones:
//! ink for chrome, paper for inverted rows, accent for the one
//! semantic channel (the live generation).

use eframe::egui;
use std::collections::HashMap;

pub const PAPER: egui::Color32 = egui::Color32::WHITE;
pub const INK: egui::Color32 = egui::Color32::BLACK;
/* #047b62 = oklch(0.52 0.10 172); 4.87:1 on white (wcag 1.4.3) */
pub const ACCENT: egui::Color32 = egui::Color32::from_rgb(0x04, 0x7b, 0x62);

#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
pub enum Tone {
    Ink,
    Paper,
    Accent,
}

const DOC: [&str; 16] = [
    "................",
    "...XXXXXXXXX....",
    "...X.......XX...",
    "...X.......X.X..",
    "...X.......XXXX.",
    "...X..........X.",
    "...X..X.X.X...X.",
    "...X..........X.",
    "...X..X.X.X...X.",
    "...X..........X.",
    "...X..X.X.X...X.",
    "...X..........X.",
    "...X..........X.",
    "...X..........X.",
    "...XXXXXXXXXX...",
    "................",
];

const DNA: [&str; 16] = [
    "................",
    "......XX........",
    ".....X..X.......",
    "....X....X......",
    "...X......X.....",
    "...X......X.....",
    "....X....X......",
    ".....X..X.......",
    "......XX........",
    ".....X..X.......",
    "....X....X......",
    "...X......X.....",
    "....X....X......",
    ".....X..X.......",
    "......XX........",
    "................",
];

const FOLDER: [&str; 16] = [
    "................",
    "..XXXXX.........",
    "..X...X.........",
    "..X...XXXXXXXXX.",
    "..X..........X..",
    "..X..........X..",
    "..X..........X..",
    "..X..........X..",
    "..X..........X..",
    "..X..........X..",
    "..X..........X..",
    "..X..........X..",
    "..XXXXXXXXXXXX..",
    "................",
    "................",
    "................",
];

const KEY: [&str; 16] = [
    "................",
    "................",
    "....XXXX........",
    "...X....X.......",
    "..X......X......",
    "..X......X......",
    "..X......X......",
    "...X....X.......",
    "....XXXX........",
    "......XX........",
    "......XX........",
    "......XX........",
    "......XXXX......",
    "......XX........",
    "......XXX.......",
    "................",
];

const TRASH: [&str; 16] = [
    "................",
    "......XXXX......",
    "......X..X......",
    "...XXXXXXXXXX...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...X..X..X..X...",
    "...XXXXXXXXXX...",
    "................",
    "................",
];

/* owner-approved logo (variant A): the engine helix in a square
   frame; doubles as the app mark in the title bar */
const LOGO_A: [&str; 16] = [
    "XXXXXXXXXXXXXXXX",
    "X......XX......X",
    "X.....X..X.....X",
    "X....X....X....X",
    "X...X......X...X",
    "X...X......X...X",
    "X....X....X....X",
    "X.....X..X.....X",
    "X......XX......X",
    "X.....X..X.....X",
    "X....X....X....X",
    "X...X......X...X",
    "X....X....X....X",
    "X.....X..X.....X",
    "X......XX......X",
    "XXXXXXXXXXXXXXXX",
];

pub fn map(name: &str) -> Option<&'static [&'static str; 16]> {
    match name {
        "doc" => Some(&DOC),
        "dna" => Some(&DNA),
        "folder" => Some(&FOLDER),
        "key" => Some(&KEY),
        "trash" => Some(&TRASH),
        "logo" => Some(&LOGO_A),
        _ => None,
    }
}

pub struct Textures {
    inner: HashMap<(String, Tone), egui::TextureHandle>,
}

impl Textures {
    pub fn load(cc: &egui::Context) -> Textures {
        let names = ["doc", "dna", "folder", "key", "trash", "logo"];
        let tones = [Tone::Ink, Tone::Paper, Tone::Accent];
        let mut tex = HashMap::new();
        for n in names {
            let m = map(n).unwrap();
            for t in tones {
                let mut img = egui::ColorImage::new([16, 16], egui::Color32::TRANSPARENT);
                let color = match t {
                    Tone::Ink => INK,
                    Tone::Paper => PAPER,
                    Tone::Accent => ACCENT,
                };
                for (y, row) in m.iter().enumerate() {
                    for (x, c) in row.chars().enumerate() {
                        if c == 'X' {
                            img.pixels[y * 16 + x] = color;
                        }
                    }
                }
                let handle = cc.load_texture(
                    format!("px_{n}_{:?}", t),
                    img,
                    egui::TextureOptions::LINEAR,
                );
                tex.insert((n.to_string(), t), handle);
            }
        }
        Textures { inner: tex }
    }

    pub fn get(&self, name: &str, tone: Tone) -> &egui::TextureHandle {
        self.inner.get(&(name.to_string(), tone)).expect("pixel texture")
    }
}
