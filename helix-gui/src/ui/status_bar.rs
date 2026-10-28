//! dwm bar: cwd left, version center, generation right

use eframe::egui::{self, RichText};

use crate::style::colors;

pub fn show(
    ui: &mut egui::Ui, cwd: &str, gen: Option<u32>, keychain_size: usize,
) {
    ui.monospace(RichText::new(format!(" {}", cwd)).color(colors::TEXT_DIM));
    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
        match gen {
            Some(g) => ui.monospace(RichText::new(format!("gen {g} "))
                .color(colors::SUCCESS)),
            None => ui.monospace(RichText::new("no seed ").color(colors::TEXT_DIM)),
        };
        ui.monospace(RichText::new(format!("kch {keychain_size} ")).color(colors::TEXT_DIM));
        ui.monospace(RichText::new(format!("helix-gui {} ", env!("CARGO_PKG_VERSION")))
            .color(colors::TEXT_DIM));
    });
}
