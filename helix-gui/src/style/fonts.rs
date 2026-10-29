//! type: IBM Plex Sans for ui, IBM Plex Mono for data, with a DejaVu
//! fallback when the Plex family is not installed (it is not, on a stock
//! Debian box). constitution: sentence case, 11 px tracked micro-heads.

use eframe::egui;
use std::path::Path;

pub const FONT_UI: &str = "helix_ui";
pub const FONT_MONO: &str = "helix_mono";
pub const FONT_SIZE_BODY: f32 = 13.0;
pub const FONT_SIZE_DIM: f32 = 11.0;
pub const FONT_SIZE_HEAD: f32 = 18.0;

const SANS_CANDIDATES: [&str; 5] = [
    "/usr/share/fonts/truetype/ibm-plex/IBMPlexSans-Regular.ttf",
    "/usr/share/fonts/opentype/ibm-plex/IBMPlexSans-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
];

const MONO_CANDIDATES: [&str; 5] = [
    "/usr/share/fonts/truetype/ibm-plex/IBMPlexMono-Regular.ttf",
    "/usr/share/fonts/opentype/ibm-plex/IBMPlexMono-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
];

fn load_first(paths: &[&str]) -> Option<egui::FontData> {
    for path in paths {
        if Path::new(path).exists() {
            if let Ok(data) = std::fs::read(path) {
                return Some(egui::FontData::from_owned(data));
            }
        }
    }
    None
}

pub fn apply(ctx: &egui::Context) {
    let mut fonts = egui::FontDefinitions::default();
    if let Some(sans) = load_first(&SANS_CANDIDATES) {
        fonts.font_data.insert(FONT_UI.to_owned(), sans);
        fonts
            .families
            .entry(egui::FontFamily::Proportional)
            .or_default()
            .insert(0, FONT_UI.to_owned());
    }
    if let Some(mono) = load_first(&MONO_CANDIDATES) {
        fonts.font_data.insert(FONT_MONO.to_owned(), mono);
        fonts
            .families
            .entry(egui::FontFamily::Monospace)
            .or_default()
            .insert(0, FONT_MONO.to_owned());
    }
    ctx.set_fonts(fonts);

    let mut style = (*ctx.style()).clone();
    style.text_styles = [
        (
            egui::TextStyle::Heading,
            egui::FontId::new(FONT_SIZE_HEAD, egui::FontFamily::Proportional),
        ),
        (
            egui::TextStyle::Body,
            egui::FontId::new(FONT_SIZE_BODY, egui::FontFamily::Proportional),
        ),
        (
            egui::TextStyle::Monospace,
            egui::FontId::new(FONT_SIZE_BODY, egui::FontFamily::Monospace),
        ),
        (
            egui::TextStyle::Button,
            egui::FontId::new(FONT_SIZE_BODY, egui::FontFamily::Proportional),
        ),
        (
            egui::TextStyle::Small,
            egui::FontId::new(FONT_SIZE_DIM, egui::FontFamily::Proportional),
        ),
    ]
    .into_iter()
    .collect();
    ctx.set_style(style);
}

/* data (keys, hashes, paths) is always monospace so columns line up */
pub fn font_id(size: f32) -> egui::FontId {
    egui::FontId::new(size, egui::FontFamily::Monospace)
}

/* tracked 11 px micro-head, the only uppercase text in the ui */
pub fn micro_head(text: &str) -> egui::RichText {
    egui::RichText::new(text.to_uppercase())
        .font(egui::FontId::new(
            FONT_SIZE_DIM,
            egui::FontFamily::Proportional,
        ))
        .color(crate::style::colors::theme().ink2)
}
