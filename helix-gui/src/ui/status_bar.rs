//! dwm bar: cwd left, version center, generation right — with blinking cursor

use eframe::egui::{self, RichText};

use crate::style::colors;

pub fn show(
    ui: &mut egui::Ui, cwd: &str, gen: Option<u32>, keychain_size: usize,
    cursor_blink: bool,
) {
    let cursor = if cursor_blink { "█" } else { " " };
    ui.monospace(
        RichText::new(format!(" {} {}", cwd, cursor)).color(colors::text_dim())
    );
    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
        match gen {
            Some(g) => ui.monospace(RichText::new(format!(" gen {g} ")).color(colors::OK)),
            None => ui.monospace(RichText::new(" no seed ").color(colors::text_dim())),
        };
        ui.monospace(RichText::new(format!(" kch {} ", keychain_size)).color(colors::text_dim()));
        ui.monospace(RichText::new(format!(" helix-gui {} ", env!("CARGO_PKG_VERSION")))
            .color(colors::text_dim()));
    });
}