//! lain font config — monospace, regular only, crisp pixels.
//! uses bundled font if available, falls back to system monospace.

use eframe::egui;

pub const FONT_FAMILY: &str = "monospace"; // egui will pick system monospace (Fira Code / JetBrains Mono / DejaVu Sans Mono)
pub const FONT_SIZE_BODY: f32 = 13.0;
pub const FONT_SIZE_DIM: f32 = 11.0;
pub const FONT_SIZE_GEN: f32 = 18.0;

pub fn apply(ctx: &egui::Context) {
    // no bold anywhere — phosphor terminals don't bold
    let mut fonts = egui::FontDefinitions::default();
    
    // add a monospace font if not present
    if !fonts.font_data.contains_key("lain_mono") {
        // use system font as fallback since we don't have bundled font
        fonts.families
            .entry(egui::FontFamily::Monospace)
            .or_default()
            .insert(0, FONT_FAMILY.to_owned());
    }
    
    // also update proportional family to use same font for consistency
    fonts.families
        .entry(egui::FontFamily::Proportional)
        .or_default()
        .insert(0, FONT_FAMILY.to_owned());
    
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