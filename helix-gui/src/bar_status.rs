//! status bar: cwd, blinking block cursor, last-operation journal,
//! generation and audio state on the right.

use eframe::egui::{self, RichText};

use crate::theme;

pub fn show(
    ui: &mut egui::Ui, cwd: &str, gen: Option<u32>, kch: usize,
    cursor_on: bool, journal: Option<&str>, audio_ok: bool, sound_on: bool,
) {
    let t = theme::theme();
    let s = crate::i18n::s();
    let cursor = if cursor_on { "█" } else { " " };
    ui.horizontal(|ui| {
        ui.label(
            RichText::new(format!("{cwd} {cursor}"))
                .font(crate::fonts::font_id(12.0, true))
                .color(t.ink2),
        );
        if let Some(j) = journal {
            ui.add_space(12.0);
            ui.label(
                RichText::new(j)
                    .font(crate::fonts::font_id(12.0, true))
                    .color(t.ink2),
            );
        }
        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
            /* the running build is visible at a glance: a stale process
               cannot masquerade as the installed one */
            ui.label(
                RichText::new(concat!(
                    "v",
                    env!("CARGO_PKG_VERSION"),
                    "-"
                ))
                .font(crate::fonts::font_id(12.0, true))
                .color(t.ink2),
            );
            ui.label(
                RichText::new(format!("helix-gui {}", env!("CARGO_PKG_VERSION")))
                    .small()
                    .color(t.ink2),
            );
            ui.add_space(12.0);
            ui.label(
                RichText::new(if sound_on && audio_ok {
                    format!("{}: {}", s.audio, s.on)
                } else if sound_on {
                    format!("{}: {}", s.audio, s.audio_off)
                } else {
                    format!("{}: {}", s.audio, s.off)
                })
                .small()
                .color(t.ink2),
            );
            ui.add_space(12.0);
            match gen {
                Some(g) => ui.label(
                    RichText::new(format!("{} {g}", s.gen_short))
                        .font(crate::fonts::font_id(12.0, true))
                        .color(t.ok),
                ),
                None => ui.label(
                    RichText::new(s.no_seed)
                        .font(crate::fonts::font_id(12.0, true))
                        .color(t.ink2),
                ),
            };
            ui.add_space(12.0);
            ui.label(
                RichText::new(format!("{} {kch}", s.kch))
                    .font(crate::fonts::font_id(12.0, true))
                    .color(t.ink2),
            );
        });
    });
}
