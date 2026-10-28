//! helix-gui: dwm tiling style, sun purple palette, lain accents.
//! the c core runs on worker threads; the ui stays responsive.

#[path = "core/helix_wrapper.rs"]
mod wrapper;

mod style;
mod ui;

use eframe::egui;
use std::sync::mpsc;

use ui::encryption_dialog::{Dialog, Mode};
use ui::file_manager::{self, FileEntry};
use ui::key_manager::KeyState;
use ui::settings::Settings;
use style::effects::Glitch;

enum Job {
    Encrypt { inp: String, out: String, m: String, gen: u32 },
    Decrypt { inp: String, out: String, m: String },
    Evolve { m: String },
    List { m: String },
}

struct JobResult {
    kind: &'static str,
    res: Result<String, String>,
}

struct App {
    cwd: String,
    files: Vec<FileEntry>,
    selected: usize,
    settings: Settings,
    keys: KeyState,
    dialog: Dialog,
    mnemonic: Option<String>,
    rx: Option<mpsc::Receiver<JobResult>>,
    status: Option<(bool, String)>,
    glitch: Option<Glitch>,
}

impl Default for App {
    fn default() -> Self {
        App {
            cwd: std::env::current_dir()
                .map(|p| p.to_string_lossy().into_owned())
                .unwrap_or_default(),
            files: Vec::new(),
            selected: 0,
            settings: Settings::default(),
            keys: KeyState { list: None, busy: false },
            dialog: Dialog::new(),
            mnemonic: None,
            rx: None,
            status: None,
            glitch: None,
        }
    }
}

impl App {
    fn spawn(&mut self, job: Job) {
        let (tx, rx) = mpsc::channel();
        self.rx = Some(rx);
        self.dialog.busy = true;
        std::thread::spawn(move || {
            let (kind, res) = match job {
                Job::Encrypt { inp, out, m, gen } => (
                    "encrypt",
                    wrapper::encrypt(&inp, &out, &m, None, gen)
                        .map(|_| format!("{inp} -> {out} (gen {gen})")),
                ),
                Job::Decrypt { inp, out, m } => (
                    "decrypt",
                    wrapper::decrypt(&inp, &out, &m, None)
                        .map(|_| format!("{inp} -> {out}")),
                ),
                Job::Evolve { m } => (
                    "evolve",
                    wrapper::evolve(&m, None).map(|(g, k)| {
                        format!("generation {g}: {}", wrapper::hex(&k))
                    }),
                ),
                Job::List { m } => (
                    "list",
                    wrapper::list_generations(&m, None).map(|l| {
                        format!("{} generations, gen {}", l.keys.len(), l.current_gen)
                    }),
                ),
            };
            let _ = tx.send(JobResult { kind, res });
        });
    }

    fn rescan(&mut self) {
        self.files = file_manager::scan(std::path::Path::new(&self.cwd));
    }

    fn open_dialog(&mut self, mode: Mode) {
        self.dialog.mode = Some(mode);
        self.dialog.error = None;
        self.dialog.busy = self.rx.is_some();
        if self.dialog.mnemonic.is_empty() {
            if let Some(m) = &self.mnemonic {
                self.dialog.mnemonic = m.clone();
            }
        }
    }

    fn handle_result(&mut self, r: JobResult) {
        self.dialog.busy = false;
        match r.res {
            Ok(msg) => {
                self.status = Some((false, msg));
                if self.mnemonic.is_none() {
                    self.mnemonic = Some(self.dialog.mnemonic.clone());
                }
                match r.kind {
                    "evolve" | "list" => {
                        if let Some(m) = self.mnemonic.clone() {
                            self.keys.list =
                                wrapper::list_generations(&m, None).ok();
                        }
                    }
                    _ => self.rescan(),
                }
            }
            Err(e) => {
                let now = std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .map(|d| d.as_secs_f64())
                    .unwrap_or(0.0);
                self.glitch = Some(Glitch::trigger(now));
                self.status = Some((true, e.clone()));
                self.dialog.error = Some(e);
            }
        }
    }
}

impl eframe::App for App {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        style::apply(ctx);
        let now = ctx.input(|i| i.time);

        // poll the worker
        if let Some(rx) = &self.rx {
            let mut got = None;
            while let Ok(r) = rx.try_recv() {
                got = Some(r);
            }
            if let Some(r) = got {
                self.rx = None;
                self.handle_result(r);
            } else if self.dialog.busy {
                ctx.request_repaint_after(std::time::Duration::from_millis(100));
            }
        }

        // keyboard, only when no text field owns the keys
        let keyboard_free = !ctx.wants_keyboard_input();
        if keyboard_free && self.dialog.mode.is_none() {
            let (up, down, act, enc, del) = ctx.input(|i| {
                (
                    i.key_pressed(egui::Key::K),
                    i.key_pressed(egui::Key::J),
                    i.key_pressed(egui::Key::Enter),
                    i.key_pressed(egui::Key::E),
                    i.key_pressed(egui::Key::Delete),
                )
            });
            if down && self.selected + 1 < self.files.len() {
                self.selected += 1;
            }
            if up && self.selected > 0 {
                self.selected -= 1;
            }
            if let Some(f) = self.files.get(self.selected) {
                let path = f.path.clone();
                if enc {
                    self.dialog.target_gen =
                        self.keys.list.as_ref().map_or(0, |l| l.current_gen);
                    self.open_dialog(Mode::Encrypt(path));
                } else if del {
                    self.open_dialog(Mode::Delete(path));
                } else if act {
                    if f.is_dna {
                        self.open_dialog(Mode::Decrypt(path));
                    } else {
                        self.dialog.target_gen =
                            self.keys.list.as_ref().map_or(0, |l| l.current_gen);
                        self.open_dialog(Mode::Encrypt(path));
                    }
                }
            }
        }

        // right tile: key manager
        egui::SidePanel::right("keys")
            .exact_width(300.0)
            .resizable(false)
            .show(ctx, |ui| {
                let mut on_evolve = false;
                let mut on_export = false;
                let mut on_reload = false;
                let m = self.mnemonic.clone();
                ui::key_manager::show(
                    ui,
                    &mut self.keys,
                    m.as_deref(),
                    self.settings.glow,
                    &mut on_evolve,
                    &mut on_export,
                    &mut on_reload,
                );
                if on_evolve {
                    if let Some(m) = self.mnemonic.clone() {
                        self.spawn(Job::Evolve { m });
                    }
                }
                if on_reload {
                    if let Some(m) = self.mnemonic.clone() {
                        self.spawn(Job::List { m });
                    }
                }
                if on_export {
                    if let Some(m) = &self.mnemonic {
                        self.status =
                            Some((false, format!("seed: {m}")));
                    }
                }
            });

        // bottom tile: settings
        egui::TopBottomPanel::bottom("settings").exact_height(64.0)
            .frame(egui::Frame::side_top_panel(&ctx.style()).fill(style::colors::BG).stroke(egui::Stroke::new(1.0, style::colors::BORDER)))
            .show(ctx, |ui| {
                ui::settings::show(ui, &mut self.settings);
            });

        // dwm bar
        egui::TopBottomPanel::bottom("status").exact_height(24.0)
            .frame(egui::Frame::side_top_panel(&ctx.style()).fill(style::colors::SURFACE))
            .show(ctx, |ui| {
                let gen = self.keys.list.as_ref().map(|l| l.current_gen);
                let n = self.keys.list.as_ref().map_or(0, |l| l.keys.len());
                ui.columns(2, |cols| {
                    ui::status_bar::show(&mut cols[0], &self.cwd, None, 0);
                    ui::status_bar::show(&mut cols[1], "", gen, n);
                });
            });

        // center: file manager
        egui::CentralPanel::default()
            .frame(egui::Frame::central_panel(&ctx.style()).fill(style::colors::BG))
            .show(ctx, |ui| {
                ui.horizontal(|ui| {
                    ui.heading(egui::RichText::new("files").color(style::colors::ACCENT));
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        if ui.button("decrypt").clicked() {
                            if let Some(f) = self.files.get(self.selected) {
                                if f.is_dna {
                                    let p = f.path.clone();
                                    self.open_dialog(Mode::Decrypt(p));
                                }
                            }
                        }
                        if ui.button("encrypt").clicked() {
                            if let Some(f) = self.files.get(self.selected) {
                                if !f.is_dna {
                                    let p = f.path.clone();
                                    self.dialog.target_gen =
                                        self.keys.list.as_ref().map_or(0, |l| l.current_gen);
                                    self.open_dialog(Mode::Encrypt(p));
                                }
                            }
                        }
                    });
                });
                ui.add_space(4.0);
                ui::file_manager::show(
                    ui,
                    &self.files,
                    &mut self.selected,
                    self.settings.glow,
                );
                if let Some((err, msg)) = &self.status {
                    ui.add_space(4.0);
                    let color = if *err {
                        style::colors::ERROR
                    } else {
                        style::colors::SUCCESS
                    };
                    ui.monospace(
                        egui::RichText::new(msg.clone()).color(color),
                    );
                }
            });

        // modal
        let mut on_confirm = false;
        let mut on_close = false;
        let mut out_msg: Option<String> = None;
        ui::encryption_dialog::show(
            ctx,
            &mut self.dialog,
            now,
            &mut out_msg,
            &mut on_confirm,
            &mut on_close,
        );
        if on_close {
            self.dialog.mode = None;
            self.dialog.error = None;
        }
        if on_confirm {
            self.confirm_dialog();
        }

        if self.settings.scanlines {
            style::effects::scanlines(ctx);
        }
    }
}

impl App {
    fn confirm_dialog(&mut self) {
        let m = self.dialog.mnemonic.trim().to_string();
        if m.split_whitespace().count() < 12 {
            self.dialog.error =
                Some("mnemonic too short: 12 or 24 words".into());
            return;
        }
        self.mnemonic = Some(m.clone());
        let Some(mode) = self.dialog.mode.take() else { return };
        self.dialog.error = None;
        match mode {
            Mode::Encrypt(inp) => {
                let out = format!("{inp}.dna");
                self.spawn(Job::Encrypt { inp, out, m, gen: self.dialog.target_gen });
            }
            Mode::Decrypt(inp) => {
                let out = if let Some(stripped) = inp.strip_suffix(".dna") {
                    stripped.to_string()
                } else {
                    format!("{inp}.dec")
                };
                self.spawn(Job::Decrypt { inp, out, m });
            }
            Mode::Delete(path) => {
                match std::fs::remove_file(&path) {
                    Ok(()) => {
                        self.status = Some((false, format!("deleted {path}")));
                        self.rescan();
                    }
                    Err(e) => {
                        self.status = Some((true, format!("delete failed: {e}")));
                    }
                }
            }
        }
    }
}

fn main() -> eframe::Result<()> {
    // the c core resolves the connectome relative to the cwd; gui runs
    // from helix-gui/ inside the repo, so point it at the vendored file
    if !std::path::Path::new("vendor/petri/data/herm_302.csv").exists() {
        std::env::set_var("HELIX_CONNECTOME", "../vendor/petri/data/herm_302.csv");
    }
    let mut app = App::default();
    app.rescan();
    let opts = eframe::NativeOptions {
        initial_window_size: Some(egui::vec2(1040.0, 660.0)),
        ..Default::default()
    };
    eframe::run_native("helix", opts, Box::new(move |cc| {
        style::apply(&cc.egui_ctx);
        Box::new(app) as Box<dyn eframe::App>
    }))
}

