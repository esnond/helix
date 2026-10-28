pub mod colors;
pub mod effects;

use eframe::egui;

/// apply the palette and force full monospace typography
pub fn apply(ctx: &egui::Context) {
    let mut style = (*ctx.style()).clone();
    let v = &mut style.visuals;
    v.panel_fill = colors::BG;
    v.window_fill = colors::SURFACE;
    v.extreme_bg_color = colors::BG;
    v.override_text_color = Some(colors::TEXT);
    v.selection.bg_fill = colors::PRIMARY;
    v.selection.stroke = egui::Stroke::new(1.0, colors::ACCENT);
    v.widgets.noninteractive.bg_fill = colors::BG;
    v.widgets.noninteractive.fg_stroke = egui::Stroke::new(1.0, colors::TEXT);
    v.widgets.noninteractive.bg_stroke = egui::Stroke::new(1.0, colors::BORDER);
    v.widgets.inactive.bg_fill = colors::SURFACE;
    v.widgets.inactive.weak_bg_fill = colors::SURFACE;
    v.widgets.inactive.fg_stroke = egui::Stroke::new(1.0, colors::TEXT_DIM);
    v.widgets.inactive.bg_stroke = egui::Stroke::new(1.0, colors::BORDER);
    v.widgets.hovered.weak_bg_fill = colors::SURFACE;
    v.widgets.hovered.bg_stroke = egui::Stroke::new(1.0, colors::SECONDARY);
    v.widgets.active.weak_bg_fill = colors::PRIMARY;
    v.widgets.active.bg_stroke = egui::Stroke::new(1.0, colors::ACCENT);
    for ty in [
        egui::TextStyle::Body,
        egui::TextStyle::Button,
        egui::TextStyle::Small,
        egui::TextStyle::Heading,
        egui::TextStyle::Monospace,
    ] {
        let size = match ty {
            egui::TextStyle::Heading => 14.0,
            egui::TextStyle::Body | egui::TextStyle::Monospace => 12.0,
            _ => 12.0,
        };
        style.text_styles.insert(ty, egui::FontId::monospace(size));
    }
    ctx.set_style(style);
}
