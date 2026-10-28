//! settings strip: visual effect toggles. the pbkdf2 iteration count
//! stays a compile time constant (suckless config policy) so it is
//! shown, not a slider.

use eframe::egui::{self, RichText, Slider};

use crate::style::colors;

#[derive(Clone, Copy)]
pub struct Settings {
    pub scanlines: bool,
    pub glow: f32,
}

impl Default for Settings {
    fn default() -> Self {
        Settings { scanlines: true, glow: 1.0 }
    }
}

pub fn show(ui: &mut egui::Ui, s: &mut Settings) {
    ui.columns(3, |cols| {
        cols[0].monospace(RichText::new("effects").color(colors::ACCENT));
        cols[0].checkbox(&mut s.scanlines, "scanlines");
        cols[1].monospace(RichText::new("glow").color(colors::ACCENT));
        cols[1].add(
            Slider::new(&mut s.glow, 0.0..=2.0)
                .text("")
                .show_value(false),
        );
        cols[2].monospace(RichText::new("core").color(colors::ACCENT));
        cols[2].monospace(RichText::new(
            "pbkdf2 600k iters (compile time, see src/helix_config.h)",
        )
        .color(colors::TEXT_DIM));
    });
}
