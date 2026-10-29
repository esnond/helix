//! the keys column: phrase entry, the dendrite chain, and the settings
//! toggles. the phrase button is primary — without it nothing works.

use eframe::egui::{self, pos2, vec2, RichText, Sense, Stroke};

use crate::i18n;
use crate::theme;
use crate::wrapper::KeyList;

pub struct KeyState {
    pub list: Option<KeyList>,
}

pub enum Action {
    None,
    Seed,
    Evolve,
    Reveal,
    Reload,
    TogglePolarity,
    ToggleLang,
    ToggleSound,
}

fn drift_bits(a: &[u8; 32], b: &[u8; 32]) -> u32 {
    let mut d = 0;
    for i in 0..32 {
        d += (a[i] ^ b[i]).count_ones();
    }
    d
}

/// chain viz: one node per generation, notches = drift from the parent
/// (hamming/8, capped), the current generation the only filled node.
/// hover shows the honest re-derive cost.
fn dendrite(ui: &mut egui::Ui, list: &KeyList) {
    let t = theme::theme();
    let n = list.keys.len().min(48);
    if n == 0 {
        return;
    }
    let row_h = 15.0;
    let width = ui.available_width();
    let (rect, resp) =
        ui.allocate_exact_size(vec2(width, n as f32 * row_h + 6.0), Sense::hover());
    let p = ui.painter_at(rect);
    let hover = resp.hover_pos().map(|pos| {
        ((pos.y - rect.top() - 3.0) / row_h).floor().max(0.0) as usize
    });
    if let Some(i) = hover {
        if i < n {
            resp.on_hover_text(format!(
                "{} {}: 600k pbkdf2 + 10000 шагов ≈ 3.4 s\n(gen {} перезапускается с фразы)",
                i18n::s().generation, i, i
            ));
        }
    }
    let spine_x = rect.left() + 8.0;
    p.line_segment(
        [pos2(spine_x, rect.top() + row_h * 0.5), pos2(spine_x, rect.top() + row_h * (n as f32 - 0.5))],
        Stroke::new(theme::HAIRLINE, t.line2),
    );
    for i in 0..n {
        let y = rect.top() + 3.0 + row_h * (i as f32 + 0.5);
        let is_cur = i as u32 == list.current_gen;
        if hover == Some(i) {
            p.rect_filled(
                egui::Rect::from_min_size(pos2(rect.left(), y - row_h * 0.5), vec2(rect.width(), row_h)),
                2.0,
                theme::surface_hover(),
            );
        }
        let c = pos2(spine_x, y);
        if is_cur {
            p.circle_filled(c, 3.6, t.accent);
        } else {
            p.circle_stroke(c, 2.4, Stroke::new(theme::HAIRLINE, t.ink2));
        }
        if i > 0 {
            let d = (drift_bits(&list.keys[i], &list.keys[i - 1]) / 8).min(6);
            for k in 0..d {
                let x = spine_x + 10.0 + k as f32 * 2.4;
                p.line_segment([pos2(x, y - 2.4), pos2(x, y + 2.4)], Stroke::new(theme::HAIRLINE, t.line2));
            }
        }
        let color = if is_cur { t.ink } else { t.ink2 };
        p.text(
            pos2(spine_x + 12.0, y),
            egui::Align2::LEFT_CENTER,
            format!("{:>3}  {}", i, crate::wrapper::hex(&list.keys[i][..5])),
            crate::fonts::font_id(12.0, true),
            color,
        );
    }
}

pub fn show(
    ui: &mut egui::Ui, keys: &Option<KeyList>, has_phrase: bool, sound_on: bool,
) -> Action {
    let t = theme::theme();
    let s = i18n::s();
    let mut action = Action::None;

    ui.label(theme::micro_head(s.keys));
    ui.add_space(6.0);

    match keys {
        Some(list) => {
            ui.label(
                RichText::new(format!("{} {}", s.generation, list.current_gen))
                    .font(crate::fonts::font_id(20.0, false))
                    .color(t.ink),
            );
            ui.add_space(6.0);
            egui::ScrollArea::vertical().max_height(230.0).show(ui, |ui| {
                dendrite(ui, list);
            });
        }
        None => {
            ui.label(
                RichText::new(if has_phrase { s.chain_not_loaded } else { s.no_phrase })
                    .small()
                    .color(t.ink2),
            );
        }
    }

    ui.add_space(10.0);
    if theme::btn(ui, s.seed_words, true).clicked() {
        action = Action::Seed;
    }
    ui.add_space(4.0);
    ui.add_enabled_ui(has_phrase && !matches!(action, Action::Seed), |ui| {
        if theme::btn(ui, s.evolve, false)
            .on_hover_text(s.tip_evolve)
            .clicked()
        {
            action = Action::Evolve;
        }
        if theme::btn(ui, s.reload, false).clicked() {
            action = Action::Reload;
        }
        if theme::btn(ui, s.reveal, false)
            .on_hover_text(s.tip_reveal)
            .clicked()
        {
            action = Action::Reveal;
        }
    });

    ui.add_space(10.0);
    ui.separator();
    /* settings: three toggles, still four groups on screen */
    ui.label(theme::micro_head(s.settings));
    ui.horizontal(|ui| {
        ui.label(RichText::new(s.polarity).small().color(t.ink2));
        if theme::btn(ui, if theme::is_light() { s.dark } else { s.light }, false).clicked() {
            action = Action::TogglePolarity;
        }
    });
    ui.horizontal(|ui| {
        ui.label(RichText::new(s.language).small().color(t.ink2));
        if theme::btn(ui, if i18n::lang() == i18n::Lang::Ru { "en" } else { "ru" }, false).clicked() {
            action = Action::ToggleLang;
        }
    });
    ui.horizontal(|ui| {
        ui.label(RichText::new(s.sound).small().color(t.ink2));
        let label = if sound_on { s.on } else { s.off };
        if theme::btn(ui, label, sound_on).clicked() {
            action = Action::ToggleSound;
        }
    });
    action
}
