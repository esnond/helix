//! lain/sun phosphor palette — teal on void, purple glow, no white, no blue.
//! derived from serial experiments lain navi terminal + sun microsystems purple.

use eframe::egui::Color32;

// base colors (const - from_rgb is const)
pub const BG_DEEP: Color32 = Color32::from_rgb(0x07, 0x07, 0x0f);
pub const BG_PANEL: Color32 = Color32::from_rgb(0x0d, 0x0d, 0x1a);
pub const TEXT_MAIN: Color32 = Color32::from_rgb(0x5e, 0xea, 0xd4);
pub const ACCENT: Color32 = Color32::from_rgb(0xa8, 0x55, 0xf7);
pub const ACCENT_GLOW: Color32 = Color32::from_rgb(0xc0, 0x84, 0xfc);
pub const WARN: Color32 = Color32::from_rgb(0xf4, 0x72, 0xb6);
pub const OK: Color32 = Color32::from_rgb(0x34, 0xd3, 0x99);
pub const BORDER: Color32 = Color32::from_rgb(0x1e, 0x1e, 0x3a);
pub const SECONDARY: Color32 = ACCENT_GLOW;

// surface variants (functions - from_rgba_unmultiplied is not const)
pub fn surface_hover() -> Color32 {
    Color32::from_rgba_unmultiplied(0x0d, 0x0d, 0x1a, 0xcc)
}
pub fn surface_active() -> Color32 {
    Color32::from_rgba_unmultiplied(0x1e, 0x1e, 0x3a, 0xee)
}

// scanline color (function - alpha dependent)
pub fn scanline_color() -> Color32 {
    Color32::from_rgba_unmultiplied(0x00, 0x00, 0x00, 0x1f)
}

// vignette color (const - from_rgb is const)
pub const VIGNETTE: Color32 = Color32::from_rgb(0x07, 0x07, 0x0f);

// noise grain color (function - alpha dependent)
pub fn noise_grain_color() -> Color32 {
    Color32::from_rgba_unmultiplied(0xff, 0xff, 0xff, 0x0c)
}

// text dim (function - alpha dependent)
pub fn text_dim() -> Color32 {
    Color32::from_rgba_unmultiplied(0x2d, 0xd4, 0xbf, 0x40)
}