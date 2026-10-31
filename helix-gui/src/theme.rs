//! theme: the two v5 polarities from docs/design/constitution.md.
//! darkest background #131314, lightest ink #d6d3cd, hairline 1.4 px.
//! polarity is one atomic so a settings toggle reaches every panel.

use eframe::egui::Color32;
use std::sync::atomic::{AtomicBool, Ordering};

#[derive(Clone, Copy)]
pub struct Theme {
    pub bg: Color32,
    pub bg2: Color32,
    pub field: Color32,
    pub ink: Color32,
    pub ink2: Color32,
    pub line: Color32,
    pub line2: Color32,
    pub accent: Color32,
    pub accent2: Color32,
    pub ok: Color32,
    pub warn: Color32,
    pub err: Color32,
}

/* comfort pass 2026-10-31: the near-black lifted, brass softened;
   semantics (ok/warn/err) untouched */
pub const DARK: Theme = Theme {
    bg: Color32::from_rgb(0x17, 0x17, 0x19),
    bg2: Color32::from_rgb(0x1c, 0x1c, 0x1f),
    field: Color32::from_rgb(0x21, 0x21, 0x25),
    ink: Color32::from_rgb(0xd2, 0xcf, 0xc9),
    ink2: Color32::from_rgb(0x92, 0x8f, 0x89),
    line: Color32::from_rgb(0x2b, 0x2b, 0x30),
    line2: Color32::from_rgb(0x3a, 0x3a, 0x41),
    accent: Color32::from_rgb(0xc4, 0x9f, 0x55),
    accent2: Color32::from_rgb(0x8a, 0x6f, 0x33),
    ok: Color32::from_rgb(0x7f, 0xa8, 0x8f),
    warn: Color32::from_rgb(0xc2, 0x9a, 0x5a),
    err: Color32::from_rgb(0xc2, 0x6a, 0x5a),
};

pub const LIGHT: Theme = Theme {
    bg: Color32::from_rgb(0xf0, 0xee, 0xe8),
    bg2: Color32::from_rgb(0xe9, 0xe6, 0xde),
    field: Color32::from_rgb(0xe4, 0xe1, 0xd8),
    ink: Color32::from_rgb(0x22, 0x20, 0x1d),
    ink2: Color32::from_rgb(0x64, 0x60, 0x5a),
    line: Color32::from_rgb(0xdb, 0xd5, 0xc8),
    line2: Color32::from_rgb(0xc9, 0xc2, 0xb2),
    accent: Color32::from_rgb(0x7a, 0x5c, 0x1e),
    accent2: Color32::from_rgb(0x5a, 0x45, 0x16),
    ok: Color32::from_rgb(0x3f, 0x6b, 0x52),
    warn: Color32::from_rgb(0x6f, 0x55, 0x18),
    err: Color32::from_rgb(0x9c, 0x3f, 0x2e),
};

static LIGHT_MODE: AtomicBool = AtomicBool::new(false);

pub fn set_light(light: bool) {
    LIGHT_MODE.store(light, Ordering::Relaxed);
}

pub fn is_light() -> bool {
    LIGHT_MODE.load(Ordering::Relaxed)
}

pub fn theme() -> Theme {
    if is_light() {
        LIGHT
    } else {
        DARK
    }
}

pub const HAIRLINE: f32 = 1.4;

pub fn surface_hover() -> Color32 {
    if is_light() {
        theme().field
    } else {
        Color32::from_rgba_unmultiplied(0x24, 0x24, 0x28, 0xff)
    }
}

/// primary button: filled with the dark accent, ink on top
pub fn btn(ui: &mut eframe::egui::Ui, label: &str, primary: bool) -> eframe::egui::Response {
    let t = theme();
    let (fill, text) = if primary {
        (t.accent2, t.bg)
    } else {
        (t.field, t.ink)
    };
    ui.add(
        eframe::egui::Button::new(
            eframe::egui::RichText::new(label).color(text).size(13.0),
        )
        .fill(fill)
        .min_size(eframe::egui::vec2(96.0, 26.0)),
    )
    .on_hover_cursor(eframe::egui::CursorIcon::PointingHand)
}

pub fn micro_head(text: &str) -> eframe::egui::RichText {
    eframe::egui::RichText::new(text.to_uppercase())
        .font(crate::fonts::font_id(crate::fonts::SZ_DIM, false))
        .color(theme().ink2)
}
