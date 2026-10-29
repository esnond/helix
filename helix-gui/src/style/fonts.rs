//! lain font config — monospace, regular only, crisp pixels.
//! loads DejaVu Sans Mono from system, falls back to embedded.

use eframe::egui;
use std::path::Path;

pub const FONT_FAMILY: &str = "lain_mono";
pub const FONT_SIZE_BODY: f32 = 13.0;
pub const FONT_SIZE_DIM: f32 = 11.0;
pub const FONT_SIZE_GEN: f32 = 18.0;

fn load_system_mono() -> Option<egui::FontData> {
    // try common system monospace font paths
    let candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/local/share/fonts/dejavu/DejaVuSansMono.ttf",
    ];
    for path in candidates {
        if Path::new(path).exists() {
            if let Ok(data) = std::fs::read(path) {
                return Some(egui::FontData::from_owned(data));
            }
        }
    }
    None
}

pub fn apply(ctx: &egui::Context) {
    // no bold anywhere — phosphor terminals don't bold
    let mut fonts = egui::FontDefinitions::default();
    
    // load system monospace font
    if let Some(font_data) = load_system_mono() {
        fonts.font_data.insert(FONT_FAMILY.to_owned(), font_data);
        fonts.families
            .entry(egui::FontFamily::Monospace)
            .or_default()
            .insert(0, FONT_FAMILY.to_owned());
        fonts.families
            .entry(egui::FontFamily::Proportional)
            .or_default()
            .insert(0, FONT_FAMILY.to_owned());
    }
    
    ctx.set_fonts(fonts);
    
    // set all text styles to monospace, regular weight, specified sizes
    let mut style = (*ctx.style()).clone();
    style.text_styles = [
        (egui::TextStyle::Heading, egui::FontId::new(FONT_SIZE_GEN, egui::FontFamily::Monospace)),
        (egui::TextStyle::Body, egui::FontId::new(FONT_SIZE_BODY, egui::FontFamily::Monospace)),
        (egui::TextStyle::Monospace, egui::FontId::new(FONT_SIZE_BODY, egui::FontFamily::Monospace)),
        (egui::TextStyle::Button, egui::FontId::new(FONT_SIZE_BODY, egui::FontFamily::Monospace)),
        (egui::TextStyle::Small, egui::FontId::new(FONT_SIZE_DIM, egui::FontFamily::Monospace)),
    ]
    .into_iter()
    .collect();
    
    ctx.set_style(style);
}

pub fn font_id(size: f32) -> egui::FontId {
    egui::FontId::new(size, egui::FontFamily::Monospace)
}