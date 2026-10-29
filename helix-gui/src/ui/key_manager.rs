//! key manager: right tile — asymmetric, shorter, inset.
//! current generation large, list below, evolve/export as prompt-buttons.

use eframe::egui::{self, RichText, Sense};

use crate::wrapper::{self, KeyList};
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
    // heading like a terminal prompt
    ui.monospace("> keys");
    ui.add_space(6.0);

    match (&state.list, mnemonic) {
        (Some(list), _) => {
            // current generation — large, centered
            let big = egui::FontId::monospace(18.0);
            let (rect, _) = ui.allocate_exact_size(
                egui::vec2(ui.available_width(), 48.0),
                Sense::hover(),
            );
            let p = ui.painter_at(rect);
            p.text(
                rect.center(),
                egui::Align2::CENTER_CENTER,
                format!("gen {}", list.current_gen),
                egui::FontId::monospace(18.0),
                colors::TEXT_MAIN,
            );
            effects::glow_focus(&p, rect, colors::ACCENT, glow);
            ui.add_space(4.0);
            ui.monospace(format!("{} generations stored", list.keys.len()));
            ui.add_space(6.0);
            egui::ScrollArea::vertical().max_height(180.0).show(ui, |ui| {
                for (i, k) in list.keys.iter().enumerate() {
                    let cur = i as u32 == list.current_gen;
                    let color = if i as u32 == list.current_gen {
                        colors::OK
                    } else {
                        colors::text_dim()
                    };
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
            ui.monospace(RichText::new("keychain not loaded").color(colors::text_dim()));
        }
        (None, None) => {
            ui.monospace(RichText::new("no seed entered").color(colors::text_dim()));
        }
    }
    ui.add_space(8.0);
    
    // buttons as terminal prompts: "> evolve"
    ui.horizontal(|ui| {
        ui.add_enabled_ui(mnemonic.is_some() && !state.busy, |ui| {
            if effects::draw_prompt_button(ui, "evolve", false, colors::TEXT_MAIN).clicked() {
                *on_evolve = true;
            }
            if effects::draw_prompt_button(ui, "reload", false, colors::TEXT_MAIN).clicked() {
                *on_reload = true;
            }
        });
        if effects::draw_prompt_button(ui, "export seed", false, colors::TEXT_MAIN).clicked() {
            *on_export = true;
        }
    });
    if state.busy {
        ui.add(egui::Spinner::new().size(18.0));
    }
}