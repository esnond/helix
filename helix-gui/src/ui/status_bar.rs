//! status bar: cwd, blinking block cursor, the last operation as a
//! one-line journal, generation and keychain count on the right.

use eframe::egui::{self, RichText};

use crate::style::colors;

pub fn show(
    ui: &mut egui::Ui, cwd: &str, gen: Option<u32>, keychain_size: usize,
    cursor_blink: bool, journal: Option<&str>,
) {
    let cursor = if cursor_blink { "█" } else { " " };
    ui.horizontal(|ui| {
        ui.label(
            RichText::new(format!("{} {}", cwd, cursor))
                .font(crate::style::fonts::font_id(12.0))
                .color(colors::theme().ink2),
        );
        if let Some(j) = journal {
            ui.add_space(12.0);
            ui.label(
                RichText::new(j)
                    .font(crate::style::fonts::font_id(12.0))
                    .color(colors::theme().ink2),
            );
        }
        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
            match gen {
                Some(g) => ui.label(
                    RichText::new(format!("gen {g}"))
                        .font(crate::style::fonts::font_id(12.0))
                        .color(colors::theme().ok),
                ),
                None => ui.label(
                    RichText::new("no seed")
                        .font(crate::style::fonts::font_id(12.0))
                        .color(colors::theme().ink2),
                ),
            };
            ui.add_space(12.0);
            ui.label(
                RichText::new(format!("kch {keychain_size}"))
                    .font(crate::style::fonts::font_id(12.0))
                    .color(colors::theme().ink2),
            );
            ui.add_space(12.0);
            ui.label(
                RichText::new(format!("helix-gui {}", env!("CARGO_PKG_VERSION")))
                    .font(crate::style::fonts::font_id(12.0))
                    .color(colors::theme().ink2),
            );
        });
    });
}
