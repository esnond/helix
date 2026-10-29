pub mod colors;
pub mod effects;
pub mod fonts;

use eframe::egui;

/// apply the lain phosphor palette and force full monospace typography
pub fn apply(ctx: &egui::Context) {
    // load fonts first
    fonts::apply(ctx);
    
    let mut style = (*ctx.style()).clone();
    let v = &mut style.visuals;
    
    // core palette
    v.panel_fill = colors::BG_DEEP;
    v.window_fill = colors::BG_PANEL;
    v.extreme_bg_color = colors::BG_DEEP;
    v.override_text_color = Some(colors::TEXT_MAIN);
    v.selection.bg_fill = colors::ACCENT;
    v.selection.stroke = egui::Stroke::new(1.0, colors::ACCENT_GLOW);
    
    // widgets
    v.widgets.noninteractive.bg_fill = colors::BG_DEEP;
    v.widgets.noninteractive.fg_stroke = egui::Stroke::new(1.0, colors::TEXT_MAIN);
    v.widgets.noninteractive.bg_stroke = egui::Stroke::new(1.0, colors::BORDER);
    v.widgets.inactive.bg_fill = colors::BG_PANEL;
    v.widgets.inactive.weak_bg_fill = colors::BG_PANEL;
    v.widgets.inactive.fg_stroke = egui::Stroke::new(1.0, colors::text_dim());
    v.widgets.inactive.bg_stroke = egui::Stroke::new(1.0, colors::BORDER);
    v.widgets.hovered.weak_bg_fill = colors::surface_hover();
    v.widgets.hovered.bg_stroke = egui::Stroke::new(1.0, colors::SECONDARY);
    v.widgets.active.weak_bg_fill = colors::surface_active();
    v.widgets.active.bg_stroke = egui::Stroke::new(1.0, colors::ACCENT);
    
    // text styles — all monospace, no bold
    for ty in [
        egui::TextStyle::Body,
        egui::TextStyle::Button,
        egui::TextStyle::Small,
        egui::TextStyle::Heading,
        egui::TextStyle::Monospace,
    ] {
        let size = match ty {
            egui::TextStyle::Heading => 18.0,
            egui::TextStyle::Body | egui::TextStyle::Monospace => 13.0,
            _ => 11.0,
        };
        style.text_styles.insert(ty, fonts::font_id(size));
    }
    
    ctx.set_style(style);
}