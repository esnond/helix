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
    v.panel_fill = colors::theme().bg;
    v.window_fill = colors::theme().bg2;
    v.extreme_bg_color = colors::theme().bg;
    v.override_text_color = Some(colors::theme().ink);
    v.selection.bg_fill = colors::theme().accent;
    v.selection.stroke = egui::Stroke::new(1.0, colors::theme().accent2);
    
    // widgets
    v.widgets.noninteractive.bg_fill = colors::theme().bg;
    v.widgets.noninteractive.fg_stroke = egui::Stroke::new(1.0, colors::theme().ink);
    v.widgets.noninteractive.bg_stroke = egui::Stroke::new(1.0, colors::theme().line);
    v.widgets.inactive.bg_fill = colors::theme().bg2;
    v.widgets.inactive.weak_bg_fill = colors::theme().bg2;
    v.widgets.inactive.fg_stroke = egui::Stroke::new(1.0, colors::text_dim());
    v.widgets.inactive.bg_stroke = egui::Stroke::new(1.0, colors::theme().line);
    v.widgets.hovered.weak_bg_fill = colors::surface_hover();
    v.widgets.hovered.bg_stroke = egui::Stroke::new(1.0, colors::theme().accent2);
    v.widgets.active.weak_bg_fill = colors::surface_active();
    v.widgets.active.bg_stroke = egui::Stroke::new(1.0, colors::theme().accent);
    
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