//! settings strip: visual effect toggles. the pbkdf2 iteration count
//! stays a compile time constant (suckless config policy) so it is
//! shown, not a slider.

use eframe::egui::{self, RichText, Slider};

use crate::style::colors;

#[derive(Clone, Copy)]
pub struct Settings {
    pub scanlines: bool,
    pub vignette: bool,
    pub noise_grain: bool,
    pub crt_flicker: bool,
    pub glow: f32,
}

impl Default for Settings {
    fn default() -> Self {
        Settings {
            scanlines: true,
            vignette: true,
            noise_grain: true,
            crt_flicker: true,
            glow: 1.0,
        }
    }
}

pub fn show(ui: &mut egui::Ui, s: &mut Settings) {
    ui.monospace(RichText::new("> settings").font(crate::style::fonts::font_id(13.0)).color(colors::ACCENT));
    ui.add_space(4.0);
    ui.columns(3, |cols| {
        cols[0].vertical(|ui| {
            ui.checkbox(&mut s.scanlines, RichText::new("scanlines").font(crate::style::fonts::font_id(13.0)).color(colors::TEXT_MAIN));
            ui.checkbox(&mut s.vignette, RichText::new("vignette").font(crate::style::fonts::font_id(13.0)).color(colors::TEXT_MAIN));
        });
        cols[1].vertical(|ui| {
            ui.checkbox(&mut s.noise_grain, RichText::new("noise grain").font(crate::style::fonts::font_id(13.0)).color(colors::TEXT_MAIN));
            ui.checkbox(&mut s.crt_flicker, RichText::new("crt flicker").font(crate::style::fonts::font_id(13.0)).color(colors::TEXT_MAIN));
        });
        cols[2].vertical(|ui| {
            ui.monospace(RichText::new("glow").font(crate::style::fonts::font_id(13.0)).color(colors::ACCENT));
            ui.add(Slider::new(&mut 0.0, 0.0..=2.0).text("").show_value(false));
        });
    });
    ui.add_space(4.0);
    ui.monospace(RichText::new("pbkdf2 600k iters (compile time, see src/helix_config.h)")
        .font(crate::style::fonts::font_id(11.0))
        .color(colors::text_dim()));
}