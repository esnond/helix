//! key manager: right tile. current generation big, all generations
//! listed, evolve and export seed actions.

use eframe::egui::{self, FontId, RichText};

use crate::wrapper::{self, KeyList};
use wrapper::hex;
use crate::style::{colors, effects};

pub struct KeyState {
    pub list: Option<KeyList>,
    pub busy: bool,
}

pub fn show(
    ui: &mut egui::Ui, state: &mut KeyState, mnemonic: Option<&str>,
    glow: f32, on_evolve: &mut bool, on_export: &mut bool,
    on_reload: &mut bool,
) {
    ui.heading(RichText::new("keys").color(colors::ACCENT));
    ui.add_space(6.0);

    match (&state.list, mnemonic) {
        (Some(list), _) => {
            let big = FontId::monospace(42.0);
            let (rect, _) = ui.allocate_exact_size(
                egui::vec2(ui.available_width(), 56.0),
                egui::Sense::hover(),
            );
            let p = ui.painter_at(rect);
            p.text(
                rect.center(),
                egui::Align2::CENTER_CENTER,
                format!("gen {}", list.current_gen),
                big,
                colors::PRIMARY,
            );
            effects::glow(&p, rect.shrink2(egui::vec2(40.0, 6.0)), colors::PRIMARY, glow);
            ui.add_space(4.0);
            ui.monospace(format!("{} generations stored", list.keys.len()));
            ui.add_space(6.0);
            egui::ScrollArea::vertical().max_height(180.0).show(ui, |ui| {
                for (i, k) in list.keys.iter().enumerate() {
                    let cur = i as u32 == list.current_gen;
                    let color = if cur { colors::SUCCESS } else { colors::TEXT_DIM };
                    ui.monospace(RichText::new(format!(
                        "{:>3}  {}",
                        i,
                        wrapper::hex(&k[..8])
                    ))
                    .color(color));
                }
            });
        }
        (None, Some(_)) => {
            ui.monospace(RichText::new("keychain not loaded").color(colors::TEXT_DIM));
        }
        (None, None) => {
            ui.monospace(RichText::new("no seed entered").color(colors::TEXT_DIM));
        }
    }
    ui.add_space(8.0);
    ui.horizontal(|ui| {
        ui.add_enabled_ui(mnemonic.is_some() && !state.busy, |ui| {
            if ui.button("evolve").clicked() {
                *on_evolve = true;
            }
            if ui.button("reload").clicked() {
                *on_reload = true;
            }
        });
        if ui.button("export seed").clicked() {
            *on_export = true;
        }
    });
    if state.busy {
        ui.add(egui::Spinner::new().size(18.0));
    }
}
