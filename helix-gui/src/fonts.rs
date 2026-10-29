//! type: IBM Plex Sans for ui, IBM Plex Mono for data, DejaVu fallback.
//! data (paths, keys) is always monospace; micro-heads are the only
//! uppercase text (constitution: sentence case everywhere else).

use eframe::egui;
use std::path::Path;

pub const SZ_BODY: f32 = 13.0;
pub const SZ_DIM: f32 = 11.0;
pub const SZ_HEAD: f32 = 18.0;

const SANS: [&str; 5] = [
    "/usr/share/fonts/truetype/ibm-plex/IBMPlexSans-Regular.ttf",
    "/usr/share/fonts/opentype/ibm-plex/IBMPlexSans-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
];

const MONO: [&str; 5] = [
    "/usr/share/fonts/truetype/ibm-plex/IBMPlexMono-Regular.ttf",
    "/usr/share/fonts/opentype/ibm-plex/IBMPlexMono-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
];

fn load_first(paths: &[&str]) -> Option<egui::FontData> {
    for p in paths {
        if Path::new(p).exists() {
            if let Ok(d) = std::fs::read(p) {
                return Some(egui::FontData::from_owned(d));
            }
        }
    }
    None
}

pub fn apply(ctx: &egui::Context) {
    let mut fonts = egui::FontDefinitions::default();
    if let Some(f) = load_first(&SANS) {
        fonts.font_data.insert("helix_ui".into(), f);
        fonts
            .families
            .entry(egui::FontFamily::Proportional)
            .or_default()
            .insert(0, "helix_ui".into());
    }
    if let Some(f) = load_first(&MONO) {
        fonts.font_data.insert("helix_mono".into(), f);
        fonts
            .families
            .entry(egui::FontFamily::Monospace)
            .or_default()
            .insert(0, "helix_mono".into());
    }
    ctx.set_fonts(fonts);

    let mut style = (*ctx.style()).clone();
    style.text_styles = [
        (egui::TextStyle::Heading, egui::FontId::new(SZ_HEAD, egui::FontFamily::Proportional)),
        (egui::TextStyle::Body, egui::FontId::new(SZ_BODY, egui::FontFamily::Proportional)),
        (egui::TextStyle::Monospace, egui::FontId::new(SZ_BODY, egui::FontFamily::Monospace)),
        (egui::TextStyle::Button, egui::FontId::new(SZ_BODY, egui::FontFamily::Proportional)),
        (egui::TextStyle::Small, egui::FontId::new(SZ_DIM, egui::FontFamily::Proportional)),
    ]
    .into_iter()
    .collect();
    ctx.set_style(style);
}

pub fn font_id(size: f32, mono: bool) -> egui::FontId {
    if mono {
        egui::FontId::new(size, egui::FontFamily::Monospace)
    } else {
        egui::FontId::new(size, egui::FontFamily::Proportional)
    }
}
