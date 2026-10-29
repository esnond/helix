//! lain style terminal modal: prompt for the mnemonic, show progress
//! while the c core works, glitch flash on errors.

use eframe::egui::{self, RichText};

use crate::style::{colors, effects};

pub enum Mode {
    Encrypt(String),
    Decrypt(String),
    Delete(String),
}

pub struct Dialog {
    pub mode: Option<Mode>,
    pub mnemonic: String,
    pub target_gen: u32,
    pub error: Option<String>,
    pub glitch: Option<effects::Glitch>,
    pub busy: bool,
}

impl Dialog {
    pub fn new() -> Self {
        Dialog {
            mode: None,
            mnemonic: String::new(),
            target_gen: 0,
            error: None,
            glitch: None,
            busy: false,
        }
    }
}

pub fn show(
    ctx: &egui::Context, d: &mut Dialog, now: f64, out_msg: &mut Option<String>,
    on_confirm: &mut bool, on_close: &mut bool,
) {
    let Some(mode) = &d.mode else { return };
    // dim overlay
    egui::Area::new(egui::Id::new("dna_modal_dim"))
        .order(egui::Order::Foreground)
        .fixed_pos(egui::pos2(0.0, 0.0))
        .show(ctx, |ui| {
            let rect = ui.available_rect_before_wrap();
            ui.painter().rect_filled(rect, 0.0, colors::BG_DEEP.gamma_multiply(0.65));
        });
    egui::Window::new(
        egui::RichText::new("helix").font(egui::FontId::monospace(14.0)).color(colors::ACCENT)
    )
        .collapsible(false)
        .resizable(false)
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .show(ctx, |ui| {
            let title = match mode {
                Mode::Encrypt(f) => format!("> encrypt {}", f),
                Mode::Decrypt(f) => format!("> decrypt {}", f),
                Mode::Delete(f) => format!("> delete {}?", f),
            };
            ui.monospace(RichText::new(title).color(colors::ACCENT).size(14.0));
            ui.add_space(6.0);
            if matches!(mode, Mode::Delete(_)) {
                ui.monospace(RichText::new(
                    "this deletes the file from disk. the seed words cannot restore it.",
                )
                .color(colors::WARN));
                ui.add_space(6.0);
                ui.horizontal(|ui| {
                    if effects::draw_prompt_button(ui, "confirm delete", false, colors::WARN).clicked() {
                        *on_confirm = true;
                    }
                    if effects::draw_prompt_button(ui, "cancel", false, colors::text_dim()).clicked() {
                        *on_close = true;
                    }
                });
                return;
            }
            if let Some(err) = &d.error {
                let glitch = d.glitch.as_ref().map(|g| g.active(now)).unwrap_or(false);
                let text = format!("! {err}");
                if glitch {
                    let off = d.glitch.as_ref().unwrap().offset(now);
                    let red_off = d.glitch.as_ref().unwrap().red_offset(now);
                    let blue_off = d.glitch.as_ref().unwrap().blue_offset(now);
                    // draw three times for rgb split
                    ui.monospace(RichText::new(text.clone()).color(colors::WARN.gamma_multiply(0.7)));
                    ui.painter().text(
                        ui.cursor().min + egui::vec2(red_off, 0.0),
                        egui::Align2::LEFT_TOP,
                        text.clone(),
                        egui::FontId::monospace(12.0),
                        colors::WARN.gamma_multiply(0.5),
                    );
                    ui.painter().text(
                        ui.cursor().min + egui::vec2(blue_off, 0.0),
                        egui::Align2::LEFT_TOP,
                        text.clone(),
                        egui::FontId::monospace(12.0),
                        colors::ACCENT_GLOW.gamma_multiply(0.5),
                    );
                } else {
                    ui.monospace(RichText::new(text).color(colors::WARN));
                }
                ui.add_space(6.0);
            }
            if d.busy {
                ui.horizontal(|ui| {
                    ui.add(egui::Spinner::new().size(16.0));
                    ui.monospace(RichText::new(
                        "deriving... pbkdf2 600k + 10000 connectome steps",
                    )
                    .color(colors::text_dim()));
                });
            } else {
                ui.monospace(RichText::new("seed words:").color(colors::text_dim()));
                let resp = egui::TextEdit::singleline(&mut d.mnemonic)
                    .font(egui::FontId::monospace(13.0))
                    .desired_width(420.0)
                    .hint_text("twelve or twenty four words")
                    .show(ui);
                if resp.response.lost_focus()
                    && ctx.input(|i| i.key_pressed(egui::Key::Enter))
                {
                    *on_confirm = true;
                }
                ui.add_space(8.0);
                if matches!(mode, Mode::Encrypt(_)) {
                    ui.horizontal(|ui| {
                        ui.monospace(RichText::new("generation:").color(colors::text_dim()));
                        ui.add(egui::DragValue::new(&mut d.target_gen).clamp_range(0..=999).speed(1.0));
                    });
                }
                ui.horizontal(|ui| {
                    if effects::draw_prompt_button(ui, "run", false, colors::TEXT_MAIN).clicked() {
                        *on_confirm = true;
                    }
                    if effects::draw_prompt_button(ui, "cancel", false, colors::text_dim()).clicked() {
                        *on_close = true;
                    }
                });
            }
            if let Some(msg) = out_msg {
                ui.add_space(4.0);
                ui.monospace(RichText::new(msg.clone()).color(colors::OK));
            }
        });
}