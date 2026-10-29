//! key manager: the keys column. a dendrite chain (one node per
//! generation, notches for drift from the parent), the current generation
//! as the only filled node, and plain menu rows for the actions.

use eframe::egui::{self, pos2, vec2, RichText, Sense, Stroke};

use crate::i18n;
use crate::style::{colors, effects};
use crate::wrapper::{self, KeyList};

pub struct KeyState {
    pub list: Option<KeyList>,
    pub busy: bool,
}

fn drift_bits(a: &[u8; 32], b: &[u8; 32]) -> u32 {
    let mut d = 0;
    for i in 0..32 {
        d += (a[i] ^ b[i]).count_ones();
    }
    d
}

/// chain viz: nodes are generations, notches are drift from the previous
/// key, the current generation is the only filled node. hovering a row
/// highlights that generation; hovering nothing leaves the chain calm.
fn dendrite(ui: &mut egui::Ui, list: &KeyList) {
    let t = colors::theme();
    let n = list.keys.len().min(48);
    if n == 0 {
        return;
    }
    let row_h = 15.0;
    let width = ui.available_width();
    let (rect, resp) =
        ui.allocate_exact_size(vec2(width, n as f32 * row_h + 6.0), Sense::hover());
    let p = ui.painter_at(rect);
    let hover_row = resp.hover_pos().map(|pos| {
        ((pos.y - rect.top() - 3.0) / row_h).floor().max(0.0) as usize
    });
    let spine_x = rect.left() + 8.0;
    p.line_segment(
        [
            pos2(spine_x, rect.top() + row_h * 0.5),
            pos2(spine_x, rect.top() + row_h * (n as f32 - 0.5)),
        ],
        Stroke::new(colors::HAIRLINE, t.line2),
    );
    for i in 0..n {
        let y = rect.top() + 3.0 + row_h * (i as f32 + 0.5);
        let is_cur = i as u32 == list.current_gen;
        let hovered = hover_row == Some(i);
        if hovered {
            p.rect_filled(
                egui::Rect::from_min_size(
                    pos2(rect.left(), y - row_h * 0.5),
                    vec2(rect.width(), row_h),
                ),
                2.0,
                colors::surface_hover(),
            );
        }
        let c = pos2(spine_x, y);
        if is_cur {
            p.circle_filled(c, 3.6, t.accent);
        } else {
            p.circle_stroke(c, 2.4, Stroke::new(colors::HAIRLINE, t.ink2));
        }
        // notches: drift from the parent, one tick per 8 bits, capped
        if i > 0 {
            let d = (drift_bits(&list.keys[i], &list.keys[i - 1]) / 8).min(6);
            for k in 0..d {
                let x = spine_x + 10.0 + k as f32 * 2.4;
                p.line_segment(
                    [pos2(x, y - 2.4), pos2(x, y + 2.4)],
                    Stroke::new(colors::HAIRLINE, t.line2),
                );
            }
        }
        let color = if is_cur { t.ink } else { t.ink2 };
        p.text(
            pos2(spine_x + 12.0, y),
            egui::Align2::LEFT_CENTER,
            format!("{:>3}  {}", i, wrapper::hex(&list.keys[i][..5])),
            crate::style::fonts::font_id(12.0),
            color,
        );
    }
}

pub fn show(
    ui: &mut egui::Ui, state: &mut KeyState, mnemonic: Option<&str>,
    glow: f32, on_evolve: &mut bool, on_export: &mut bool,
    on_reload: &mut bool,
) {
    let t = colors::theme();
    ui.label(crate::style::fonts::micro_head(i18n::s().keys));
    ui.add_space(6.0);

    match (&state.list, mnemonic) {
        (Some(list), _) => {
            ui.label(
                RichText::new(format!(
                    "{} {}",
                    i18n::s().generation,
                    list.current_gen
                ))
                .font(egui::FontId::proportional(20.0))
                .color(t.ink),
            );
            ui.add_space(6.0);
            egui::ScrollArea::vertical()
                .max_height(240.0)
                .show(ui, |ui| {
                    dendrite(ui, list);
                });
            if glow > 0.0 {
                effects::glow_focus(
                    ui.painter(),
                    egui::Rect::from_min_size(
                        ui.min_rect().left_top(),
                        vec2(ui.available_width(), 1.0),
                    ),
                    t.accent,
                    glow * 0.25,
                );
            }
        }
        (None, Some(_)) => {
            ui.label(RichText::new("keychain not loaded").color(t.ink2).small());
        }
        (None, None) => {
            ui.label(RichText::new("no seed entered").color(t.ink2).small());
        }
    }

    ui.add_space(10.0);
    // actions as menu rows, kbd hint on the right
    ui.horizontal(|ui| {
        ui.add_enabled_ui(mnemonic.is_some() && !state.busy, |ui| {
            if effects::draw_prompt_button(ui, i18n::s().evolve, false, t.ink).clicked() {
                *on_evolve = true;
            }
            if effects::draw_prompt_button(ui, i18n::s().reload, false, t.ink).clicked() {
                *on_reload = true;
            }
        });
    });
    ui.horizontal(|ui| {
        if effects::draw_prompt_button(ui, i18n::s().export_seed, false, t.ink).clicked() {
            *on_export = true;
        }
    });
    if state.busy {
        // no bare spinner: a reason line and a hairline instead
        ui.add_space(4.0);
        ui.label(
            RichText::new(i18n::s().busy)
                .small()
                .color(t.warn),
        );
        let (rect, _) =
            ui.allocate_exact_size(vec2(ui.available_width(), 1.4), Sense::hover());
        ui.painter().rect_filled(rect, 0.0, t.accent2);
    }
}
