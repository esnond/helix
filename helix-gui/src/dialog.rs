//! the one modal dialog: seed words, encrypt, decrypt, delete, reveal.
//! states: open -> (confirm) -> closed; errors ride on the dialog.

use eframe::egui::{self, RichText};

use crate::i18n;
use crate::theme;

#[derive(Clone)]
pub enum Kind {
    Seed,
    Encrypt(String),
    Decrypt(String),
    Delete(String),
    Reveal(String),
    /// bool: export mode (public key) instead of creation
    PairCreate(bool),
}

pub struct Dialog {
    pub kind: Kind,
    pub mnemonic: String,
    pub target_gen: u32,
    pub error: Option<String>,
    /* v3 external pair: path + its own passphrase; use_pair selects the
       v3 format in the encrypt/decrypt dialogs */
    pub use_pair: bool,
    pub pair_path: String,
    pub pair_pass: String,
    pub pair_argon2: bool,
}

pub enum Action {
    None,
    Confirm,
    Close,
}

impl Dialog {
    pub fn new(kind: Kind, current_phrase: Option<String>, pair_hint: String) -> Dialog {
        Dialog {
            mnemonic: current_phrase.unwrap_or_default(),
            target_gen: 0,
            error: None,
            kind,
            use_pair: false,
            pair_path: pair_hint,
            pair_pass: String::new(),
            pair_argon2: true,
        }
    }

    pub fn title(&self, s: &i18n::Strings) -> String {
        match &self.kind {
            Kind::Seed => format!("> {}", s.d_seed),
            Kind::Encrypt(f) => format!("> {} {f}", s.d_encrypt),
            Kind::Decrypt(f) => format!("> {} {f}", s.d_decrypt),
            Kind::Delete(f) => format!("> {} {f}?", s.d_delete),
            Kind::Reveal(_) => format!("> {}", s.seed_words),
            Kind::PairCreate(false) => format!("> {}", s.pair_create),
            Kind::PairCreate(true) => format!("> {}", s.pair_export),
        }
    }
}

pub fn show(ctx: &egui::Context, d: &mut Dialog) -> Action {
    let t = theme::theme();
    let s = i18n::s();
    let mut action = Action::None;

    /* dim overlay */
    egui::Area::new(egui::Id::new("modal_dim"))
        .order(egui::Order::Foreground)
        .fixed_pos(egui::pos2(0.0, 0.0))
        .show(ctx, |ui| {
            let rect = ui.available_rect_before_wrap();
            ui.painter().rect_filled(rect, 0.0, t.bg.gamma_multiply(0.65));
        });

    let win = egui::Window::new(
        RichText::new(d.title(s))
            .font(crate::fonts::font_id(14.0, true))
            .color(t.accent),
    )
    .collapsible(false)
    .resizable(false)
    .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0]);
    win.show(ctx, |ui| {
        match &d.kind {
            Kind::Delete(path) => {
                ui.label(RichText::new(s.d_delete_warn).color(t.err));
                ui.label(
                    RichText::new(path)
                        .font(crate::fonts::font_id(12.0, true))
                        .color(t.ink2),
                );
                ui.add_space(8.0);
                ui.horizontal(|ui| {
                    if theme::btn(ui, s.delete, true).clicked() {
                        action = Action::Confirm;
                    }
                    if theme::btn(ui, s.cancel, false).clicked() {
                        action = Action::Close;
                    }
                });
            }
            Kind::Reveal(phrase) => {
                ui.label(
                    RichText::new(phrase.clone())
                        .font(crate::fonts::font_id(13.0, true))
                        .color(t.ink),
                );
                ui.add_space(8.0);
                if theme::btn(ui, s.cancel, false).clicked() {
                    action = Action::Close;
                }
            }
            Kind::PairCreate(_) => {
                ui.label(RichText::new(s.pair_path).color(t.ink2));
                ui.add(
                    egui::TextEdit::singleline(&mut d.pair_path)
                        .font(crate::fonts::font_id(13.0, true))
                        .desired_width(420.0),
                );
                ui.add_space(6.0);
                ui.label(RichText::new(s.pair_pass).color(t.ink2));
                let resp = egui::TextEdit::singleline(&mut d.pair_pass)
                    .password(true)
                    .font(crate::fonts::font_id(13.0, true))
                    .desired_width(420.0)
                    .show(ui);
                if resp.response.lost_focus()
                    && ctx.input(|i| i.key_pressed(egui::Key::Enter))
                {
                    action = Action::Confirm;
                }
                ui.add_space(6.0);
                ui.horizontal(|ui| {
                    ui.label(RichText::new(s.pair_kdf).color(t.ink2));
                    if theme::btn(
                        ui,
                        if d.pair_argon2 { s.kdf_argon2id } else { s.kdf_pbkdf2 },
                        d.pair_argon2,
                    )
                    .clicked()
                    {
                        d.pair_argon2 = !d.pair_argon2;
                    }
                });
                if let Some(err) = &d.error {
                    ui.add_space(4.0);
                    ui.label(RichText::new(format!("! {err}")).color(t.err));
                }
                ui.add_space(8.0);
                ui.horizontal(|ui| {
                    if theme::btn(ui, s.run, true).clicked() {
                        action = Action::Confirm;
                    }
                    if theme::btn(ui, s.cancel, false).clicked() {
                        action = Action::Close;
                    }
                });
            }
            Kind::Seed | Kind::Encrypt(_) | Kind::Decrypt(_) => {
                ui.label(RichText::new(s.d_mnemonic).color(t.ink2));
                let resp = egui::TextEdit::singleline(&mut d.mnemonic)
                    .font(crate::fonts::font_id(13.0, true))
                    .desired_width(420.0)
                    .hint_text(s.d_mnemonic_hint)
                    .show(ui);
                if resp.response.lost_focus()
                    && ctx.input(|i| i.key_pressed(egui::Key::Enter))
                {
                    action = Action::Confirm;
                }
                ui.add_space(6.0);
                if matches!(d.kind, Kind::Encrypt(_) | Kind::Decrypt(_)) {
                    ui.horizontal(|ui| {
                        if theme::btn(ui, s.d_v2_phrase, !d.use_pair).clicked() {
                            d.use_pair = false;
                        }
                        if theme::btn(ui, s.d_v3_pair, d.use_pair).clicked() {
                            d.use_pair = true;
                        }
                    });
                }
                if d.use_pair && matches!(d.kind, Kind::Encrypt(_) | Kind::Decrypt(_)) {
                    ui.horizontal(|ui| {
                        ui.label(RichText::new(s.pair_path).color(t.ink2));
                        ui.add(
                            egui::TextEdit::singleline(&mut d.pair_path)
                                .font(crate::fonts::font_id(12.0, true))
                                .desired_width(300.0),
                        );
                    });
                    ui.horizontal(|ui| {
                        ui.label(RichText::new(s.pair_pass).color(t.ink2));
                        ui.add(
                            egui::TextEdit::singleline(&mut d.pair_pass)
                                .password(true)
                                .font(crate::fonts::font_id(12.0, true))
                                .desired_width(300.0),
                        );
                    });
                }
                if let Kind::Encrypt(_) = &d.kind {
                    ui.horizontal(|ui| {
                        ui.label(RichText::new(s.d_generation).color(t.ink2));
                        ui.add(
                            egui::DragValue::new(&mut d.target_gen)
                                .clamp_range(0..=999)
                                .speed(1.0),
                        );
                    });
                }
                if let Some(err) = &d.error {
                    ui.add_space(4.0);
                    ui.label(RichText::new(format!("! {err}")).color(t.err));
                }
                ui.add_space(8.0);
                ui.horizontal(|ui| {
                    let run_label = match &d.kind {
                        Kind::Seed | Kind::Reveal(_) | Kind::PairCreate(_) => s.run,
                        Kind::Encrypt(_) => s.encrypt,
                        Kind::Decrypt(_) => s.decrypt,
                        Kind::Delete(_) => s.delete,
                    };
                    if theme::btn(ui, run_label, true).clicked() {
                        action = Action::Confirm;
                    }
                    if theme::btn(ui, s.cancel, false).clicked() {
                        action = Action::Close;
                    }
                });
            }
        }
    });
    /* esc closes, but not while a text field owns the keys */
    let text_focus = ctx.memory(|m| m.focused().is_some());
    if !text_focus && ctx.input(|i| i.key_pressed(egui::Key::Escape)) {
        action = Action::Close;
    }
    action
}
