//! app state and the update loop. no crypto logic here: every operation
//! is a queued job calling the c core through the wrapper.

use std::path::PathBuf;

use eframe::egui;

use crate::bar_status;
use crate::dialog::{self, Dialog, Kind};
use crate::i18n;
use crate::jobs::{Op, Queue};
use crate::panel_keys;
use crate::panel_files::{self, FileEntry, ListState};
use crate::panel_jobs;
use crate::prefs::Prefs;
use crate::sound::{Event, SoundHandle};
use crate::theme;
use crate::wrapper::{self, KeyList};

pub struct App {
    pub prefs: Prefs,
    pub phrase: Option<String>,
    pub keys: Option<KeyList>,
    pub pair_path: String,
    pub pair_status: Option<String>,
    pub cwd: PathBuf,
    pub files: Vec<FileEntry>,
    pub list: ListState,
    pub selected: usize,
    pub dialog: Option<Dialog>,
    pub queue: Queue,
    pub sound: SoundHandle,
    pub journal: String,
    blink_on: bool,
    blink_next: f64,
    nacre_at: Option<f64>,
    hum_state: bool,
}

/// the pair option for encrypt/decrypt jobs, remembered across dialogs
fn pair_of(d: &Dialog, pair_path: &mut String) -> Result<Option<(String, String)>, String> {
    if !d.use_pair {
        return Ok(None);
    }
    let path = d.pair_path.trim().to_string();
    if path.is_empty() || d.pair_pass.is_empty() {
        return Err(i18n::s().pair_needs_path.to_string());
    }
    *pair_path = path.clone();
    Ok(Some((path, d.pair_pass.clone())))
}

#[allow(dead_code)]
fn now_s() -> f64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs_f64())
        .unwrap_or(0.0)
}

impl App {
    pub fn new(prefs: Prefs) -> App {
        let cwd = prefs
            .last_dir
            .clone()
            .filter(|d| std::path::Path::new(d).is_dir())
            .map(PathBuf::from)
            .or_else(|| std::env::current_dir().ok())
            .unwrap_or_else(|| PathBuf::from("."));
        let mut app = App {
            prefs,
            phrase: None,
            keys: None,
            pair_path: String::new(),
            pair_status: None,
            files: Vec::new(),
            list: ListState::Empty,
            selected: 0,
            dialog: None,
            queue: Queue::default(),
            sound: SoundHandle::connect(),
            journal: String::new(),
            blink_on: true,
            blink_next: 0.0,
            nacre_at: None,
            hum_state: false,
            cwd,
        };
        app.rescan();
        app
    }

    fn rescan(&mut self) {
        match panel_files::scan(&self.cwd) {
            Ok(files) => {
                self.list = if files.is_empty() { ListState::Empty } else { ListState::Ok };
                self.files = files;
            }
            Err(e) => {
                self.list = ListState::ReadFailed(e);
                self.files.clear();
            }
        }
        if self.selected >= self.files.len() {
            self.selected = self.files.len().saturating_sub(1);
        }
    }

    fn nav_into(&mut self, path: &str) {
        self.cwd = PathBuf::from(path);
        self.selected = 0;
        self.prefs.last_dir = Some(path.to_string());
        self.prefs.save();
        self.rescan();
    }

    fn nav_up(&mut self) {
        if let Some(p) = self.cwd.parent().map(|p| p.to_path_buf()) {
            self.nav_into(&p.to_string_lossy());
        }
    }

    fn open_dialog(&mut self, kind: Kind) {
        self.dialog =
            Some(Dialog::new(kind, self.phrase.clone(), self.pair_path.clone()));
    }

    fn activate(&mut self, i: usize) {
        let Some(f) = self.files.get(i) else { return };
        if f.is_dir {
            let p = f.path.clone();
            self.nav_into(&p);
        } else if f.is_dna {
            let p = f.path.clone();
            self.open_dialog(Kind::Decrypt(p));
        } else {
            let p = f.path.clone();
            self.open_dialog(Kind::Encrypt(p));
        }
    }

    fn start_job(&mut self, op: Op) {
        if self.queue.is_busy() {
            self.journal = i18n::s().busy.to_string();
            return;
        }
        let bucket = match &op {
            Op::Encrypt { inp, .. } | Op::Decrypt { inp, .. } => {
                inp.bytes().map(|b| b as u64).sum::<u64>()
            }
            _ => 0,
        };
        self.sound.play(Event::Tick { bucket });
        self.sound.play(Event::Line { ms: 1200, bucket });
        self.queue.push(op);
    }

    fn confirm_dialog(&mut self, now: f64) {
        let Some(mut d) = self.dialog.take() else { return };
        let m = d.mnemonic.trim().to_string();
        let words = m.split_whitespace().count();
        /* a typo must fail here, in the dialog, not as an async job
           error three seconds later */
        if matches!(
            d.kind,
            Kind::Seed | Kind::Encrypt(_) | Kind::Decrypt(_)
        ) && words >= 12
        {
            if let Err(e) = wrapper::phrase_valid(&m) {
                d.error = Some(e);
                self.dialog = Some(d);
                return;
            }
        }
        match &d.kind {
            Kind::Delete(path) => {
                match std::fs::remove_file(path) {
                    Ok(()) => {
                        self.journal = format!("deleted {path}");
                        self.rescan();
                        self.nacre_at = Some(now);
                    }
                    Err(e) => {
                        self.journal = format!("delete failed: {e}");
                        self.queue.failure_at = Some(now);
                    }
                }
            }
            Kind::Reveal(_) => {}
            Kind::Seed => {
                if words < 12 {
                    d.error = Some(i18n::s().d_short_phrase.to_string());
                    self.dialog = Some(d);
                    return;
                }
                self.phrase = Some(m.clone());
                self.journal = i18n::s().reload.to_string();
                self.start_job(Op::List { m });
            }
            Kind::Encrypt(inp) => {
                if words < 12 {
                    d.error = Some(i18n::s().d_short_phrase.to_string());
                    self.dialog = Some(d);
                    return;
                }
                let pair = match pair_of(&d, &mut self.pair_path) {
                    Ok(p) => p,
                    Err(e) => {
                        d.error = Some(e);
                        self.dialog = Some(d);
                        return;
                    }
                };
                let gen = d.target_gen;
                let inp = inp.clone();
                /* report.pdf -> report.dna: the last extension is
                   replaced, not appended; a file without one keeps its
                   full name */
                let out = match std::path::Path::new(&inp).file_stem() {
                    Some(stem) => format!("{}.dna", stem.to_string_lossy()),
                    None => format!("{inp}.dna"),
                };
                self.phrase = Some(m.clone());
                self.start_job(Op::Encrypt { inp, out, m, gen, pair });
            }
            Kind::Decrypt(inp) => {
                if words < 12 {
                    d.error = Some(i18n::s().d_short_phrase.to_string());
                    self.dialog = Some(d);
                    return;
                }
                let pair = match pair_of(&d, &mut self.pair_path) {
                    Ok(p) => p,
                    Err(e) => {
                        d.error = Some(e);
                        self.dialog = Some(d);
                        return;
                    }
                };
                let inp = inp.clone();
                let out = match inp.strip_suffix(".dna") {
                    Some(s) => s.to_string(),
                    None => format!("{inp}.dec"),
                };
                self.phrase = Some(m.clone());
                self.start_job(Op::Decrypt { inp, out, m, pair });
            }
            Kind::PairCreate(export) => {
                let path = d.pair_path.trim().to_string();
                let pass = d.pair_pass.clone();
                if path.is_empty() || pass.is_empty() {
                    d.error = Some(i18n::s().pair_needs_path.to_string());
                    self.dialog = Some(d);
                    return;
                }
                let res = if *export {
                    let out = format!("{path}.pub");
                    wrapper::pair_export(&path, &pass, &out)
                        .map(|()| i18n::s().pair_exported.to_string())
                } else {
                    wrapper::pair_create(&path, &pass, d.pair_argon2)
                        .map(|()| {
                            self.pair_path = path.clone();
                            i18n::s().pair_created.to_string()
                        })
                };
                match res {
                    Ok(msg) => {
                        self.pair_status = Some(msg.clone());
                        self.journal = msg;
                        self.nacre_at = Some(now);
                    }
                    Err(e) => {
                        d.error = Some(e);
                        self.dialog = Some(d);
                        return;
                    }
                }
            }
        }
    }

    fn handle_key_action(&mut self, action: panel_keys::Action, now: f64) {
        match action {
            panel_keys::Action::None => {}
            panel_keys::Action::Seed => self.open_dialog(Kind::Seed),
            panel_keys::Action::Evolve => match self.phrase.clone() {
                Some(m) => self.start_job(Op::Evolve { m }),
                None => self.open_dialog(Kind::Seed),
            },
            panel_keys::Action::Reveal => {
                if let Some(m) = self.phrase.clone() {
                    self.open_dialog(Kind::Reveal(m));
                }
            }
            panel_keys::Action::Reload => {
                if let Some(m) = self.phrase.clone() {
                    self.start_job(Op::List { m });
                }
            }
            panel_keys::Action::PairCreate => self.open_dialog(Kind::PairCreate(false)),
            panel_keys::Action::PairExport => {
                if self.pair_path.trim().is_empty() {
                    self.pair_status =
                        Some(i18n::s().pair_needs_path.to_string());
                    return;
                }
                self.open_dialog(Kind::PairCreate(true));
            }
            panel_keys::Action::TogglePolarity => {
                self.prefs.light = !self.prefs.light;
                self.prefs.save();
            }
            panel_keys::Action::ToggleLang => {
                self.prefs.lang = match self.prefs.lang {
                    i18n::Lang::Ru => i18n::Lang::En,
                    i18n::Lang::En => i18n::Lang::Ru,
                };
                self.prefs.save();
            }
            panel_keys::Action::ToggleSound => {
                self.prefs.sound = !self.prefs.sound;
                self.prefs.save();
                if self.prefs.sound && self.sound.available {
                    self.sound.play(Event::Tick { bucket: 3 });
                }
            }
        }
        let _ = now;
    }
}

impl eframe::App for App {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        theme::set_light(self.prefs.light);
        i18n::set(self.prefs.lang);
        let now = ctx.input(|i| i.time);

        /* blink: 1 s wall */
        if now >= self.blink_next {
            self.blink_on = !self.blink_on;
            self.blink_next = now + 1.0;
        }

        /* worker events */
        if let Some((op, out)) = self.queue.poll() {
            if let Some(keys) = out.keys {
                self.keys = Some(keys);
            }
            match op {
                Op::Encrypt { .. } | Op::Decrypt { .. } => self.rescan(),
                Op::Evolve { .. } | Op::List { .. } => {}
            }
            self.journal = out.message.clone();
            self.nacre_at = Some(now);
            self.sound
                .play(Event::Plate { bucket: (now * 1000.0) as u64 % 97 });
        }
        let busy = self.queue.is_busy();
        if busy != self.hum_state {
            self.sound.play(Event::Hum { on: busy });
            self.hum_state = busy;
        }
        if let Some(t0) = self.queue.failure_at.take() {
            if self.prefs.sound {
                self.sound.play(Event::Fault { bucket: (t0 * 1000.0) as u64 % 97 });
            }
        }
        if self.nacre_at.map_or(false, |t0| now - t0 > 0.6) {
            self.nacre_at = None;
        }
        if busy {
            ctx.request_repaint_after(std::time::Duration::from_millis(100));
        }

        /* keyboard accelerators, only when no text field owns the keys */
        let keyboard_free = !ctx.wants_keyboard_input();
        if keyboard_free && self.dialog.is_none() {
            if ctx.input(|i| i.modifiers.command && i.key_pressed(egui::Key::Q)) {
                self.prefs.last_dir = Some(self.cwd.to_string_lossy().into_owned());
                self.prefs.save();
                self.sound.shutdown();
                std::process::exit(0);
            }
            let (down, up, enter, enc, del, left, right) = ctx.input(|i| {
                (
                    i.key_pressed(egui::Key::J) || i.key_pressed(egui::Key::ArrowDown),
                    i.key_pressed(egui::Key::K) || i.key_pressed(egui::Key::ArrowUp),
                    i.key_pressed(egui::Key::Enter) || i.key_pressed(egui::Key::L),
                    i.key_pressed(egui::Key::E),
                    i.key_pressed(egui::Key::Delete),
                    i.key_pressed(egui::Key::H) || i.key_pressed(egui::Key::ArrowLeft),
                    i.key_pressed(egui::Key::ArrowRight),
                )
            });
            if left {
                self.nav_up();
            }
            if down && self.selected + 1 < self.files.len() {
                self.selected += 1;
            }
            if up && self.selected > 0 {
                self.selected -= 1;
            }
            if right {
                if let Some(f) = self.files.get(self.selected) {
                    if f.is_dir {
                        let p = f.path.clone();
                        self.nav_into(&p);
                    }
                }
            }
            if let Some(f) = self.files.get(self.selected).map(|f| (f.is_dir, f.is_dna, f.path.clone())) {
                let (is_dir, is_dna, path) = f;
                if enc && !is_dir && !is_dna {
                    self.open_dialog(Kind::Encrypt(path));
                } else if del && !is_dir {
                    self.open_dialog(Kind::Delete(path));
                } else if enter {
                    self.activate(self.selected);
                }
            }
        }

        /* right tile: keys column */
        let mut key_action = panel_keys::Action::None;
        egui::SidePanel::right("keys")
            .exact_width(340.0_f32.min(ctx.available_rect().width() * 0.45))
            .resizable(false)
            .show(ctx, |ui| {
                key_action = panel_keys::show(
                    ui,
                    &self.keys,
                    self.phrase.is_some(),
                    self.prefs.sound,
                    &mut self.pair_path,
                    &mut self.pair_status,
                );
            });

        /* jobs strip */
        let mut cancel_id = None;
        egui::TopBottomPanel::bottom("jobs")
            .exact_height(28.0)
            .frame(
                egui::Frame::side_top_panel(&ctx.style())
                    .fill(theme::theme().bg2)
                    .stroke(egui::Stroke::new(theme::HAIRLINE, theme::theme().line)),
            )
            .show(ctx, |ui| {
                cancel_id = panel_jobs::show(ui, &self.queue, now);
            });
        if let Some(id) = cancel_id {
            self.queue.cancel(id);
        }

        /* status bar */
        egui::TopBottomPanel::bottom("status")
            .exact_height(24.0)
            .frame(
                egui::Frame::side_top_panel(&ctx.style())
                    .fill(theme::theme().bg2)
                    .stroke(egui::Stroke::new(theme::HAIRLINE, theme::theme().line)),
            )
            .show(ctx, |ui| {
                let gen = self.keys.as_ref().map(|l| l.current_gen);
                let kch = self.keys.as_ref().map_or(0, |l| l.keys.len());
                bar_status::show(
                    ui,
                    &self.cwd.to_string_lossy(),
                    gen,
                    kch,
                    self.blink_on,
                    Some(&self.journal),
                    self.sound.available,
                    self.prefs.sound,
                );
            });

        /* central: files */
        let mut file_action = panel_files::Action::None;
        egui::CentralPanel::default()
            .frame(
                egui::Frame::central_panel(&ctx.style())
                    .fill(theme::theme().bg)
                    .inner_margin(egui::Margin::symmetric(12.0, 8.0)),
            )
            .show(ctx, |ui| {
                file_action = panel_files::show(
                    ui,
                    &self.cwd.to_string_lossy(),
                    &self.files,
                    &self.list,
                    &mut self.selected,
                    now,
                    self.nacre_at,
                );
            });

        match file_action {
            panel_files::Action::None => {}
            panel_files::Action::Up => self.nav_up(),
            panel_files::Action::Refresh => self.rescan(),
            panel_files::Action::Activate(i) => self.activate(i),
            panel_files::Action::Encrypt(p) => self.open_dialog(Kind::Encrypt(p)),
            panel_files::Action::Decrypt(p) => self.open_dialog(Kind::Decrypt(p)),
            panel_files::Action::Delete(p) => self.open_dialog(Kind::Delete(p)),
        }
        self.handle_key_action(key_action, now);

        /* the modal, last so it sits on top */
        if self.dialog.is_some() {
            let action = match &mut self.dialog {
                Some(d) => dialog::show(ctx, d),
                None => dialog::Action::None,
            };
            match action {
                dialog::Action::Confirm => self.confirm_dialog(now),
                dialog::Action::Close => self.dialog = None,
                dialog::Action::None => {}
            }
        }
    }

    fn on_close_event(&mut self) -> bool {
        self.prefs.last_dir = Some(self.cwd.to_string_lossy().into_owned());
        self.prefs.save();
        self.sound.shutdown();
        true
    }
}
