//! v5 palette: anti-glare, brass accent, two polarities.
//!
//! rules from docs/design/constitution.md: never #000 as background, never
//! #fff as ink, strokes at least 1.4 px, and the polarity is a user toggle
//! that must be persisted.
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

pub const DARK: Theme = Theme {
    bg: Color32::from_rgb(0x13, 0x13, 0x14),
    bg2: Color32::from_rgb(0x18, 0x18, 0x1a),
    field: Color32::from_rgb(0x1d, 0x1d, 0x20),
    ink: Color32::from_rgb(0xd6, 0xd3, 0xcd),
    ink2: Color32::from_rgb(0x8d, 0x8a, 0x84),
    line: Color32::from_rgb(0x26, 0x26, 0x2a),
    line2: Color32::from_rgb(0x33, 0x33, 0x38),
    accent: Color32::from_rgb(0xc8, 0xa2, 0x4a),
    accent2: Color32::from_rgb(0x8a, 0x6f, 0x33),
    ok: Color32::from_rgb(0x7f, 0xa8, 0x8f),
    warn: Color32::from_rgb(0xc2, 0x9a, 0x5a),
    err: Color32::from_rgb(0xc2, 0x6a, 0x5a),
};

pub const LIGHT: Theme = Theme {
    bg: Color32::from_rgb(0xf2, 0xef, 0xe9),
    bg2: Color32::from_rgb(0xea, 0xe6, 0xdd),
    field: Color32::from_rgb(0xe5, 0xdf, 0xd4),
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

/* polarity lives in one atomic so a settings toggle reaches every panel
   without threading a theme through every call site */
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

/* minimum stroke width, per the constitution */
pub const HAIRLINE: f32 = 1.4;

pub fn surface_hover() -> Color32 {
    if is_light() {
        theme().field
    } else {
        Color32::from_rgba_unmultiplied(0x24, 0x24, 0x28, 0xff)
    }
}

pub fn surface_active() -> Color32 {
    if is_light() {
        theme().line2
    } else {
        Color32::from_rgba_unmultiplied(0x2a, 0x2a, 0x2f, 0xff)
    }
}

/* the constitution bans scanlines-as-style; these stay transparent and
   are kept only because the effect helpers still reference them */
pub fn scanline_color() -> Color32 {
    Color32::TRANSPARENT
}

pub fn noise_grain_color() -> Color32 {
    Color32::TRANSPARENT
}

pub fn vignette() -> Color32 {
    theme().bg
}

pub fn text_dim() -> Color32 {
    theme().ink2
}
