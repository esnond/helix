//! touch-first helix in the 1-bit mac language: two tabs, 48dp rows,
//! 44dp buttons, bottom action bar (design: docs/design mac mockup,
//! logo variant A). one semantic accent — the live generation. the c
//! core is reached only through wrapper; no crypto logic in rust.

use eframe::egui;
use std::path::{Path, PathBuf};
use std::sync::mpsc::{channel, Receiver};

use crate::i18n::{self, Lang};
use crate::pixel::{self, Textures, Tone};
use crate::wrapper::{self, KeyList};

const ROW_H: f32 = 48.0;
const BTN_H: f32 = 44.0;
const SP: f32 = 6.5; /* spacing ladder 4·6.5·10.5·17·27.5 */
const SP2: f32 = 10.5;
const SP3: f32 = 17.0;

fn tr(ru: &'static str, en: &'static str) -> &'static str {
    match i18n::lang() {
        Lang::Ru => ru,
        Lang::En => en,
    }
}

#[derive(Clone, Copy, PartialEq)]
enum Tab {
    Files,
    Keys,
}

struct Entry {
    name: String,
    is_dir: bool,
    is_dna: bool,
    meta: String,
}

#[derive(Clone)]
enum Dlg {
    Encrypt { path: String },
    Decrypt { path: String },
    Confirm { path: String },
    Seed(String),
}

struct Op {
    label: &'static str,
    rx: Receiver<Result<JobOut, String>>,
}

enum JobOut {
    Msg(String),
    Keys(Box<KeyList>),
}

enum Act {
    Encrypt {
        path: String,
        gen: u32,
        pair: Option<(String, String)>,
    },
    Decrypt { path: String },
    Delete { path: String },
    Copy(String),
}

pub struct MobileApp {
    home: PathBuf,
    tab: Tab,
    entries: Vec<Entry>,
    sel: Option<usize>,
    dlg: Option<Dlg>,
    phrase: String,
    pair_pass: String,
    use_pair: bool,
    gen_sel: u32,
    pair: Option<PathBuf>,
    op: Option<Op>,
    msg: Option<(String, bool)>,
    want_seed_dlg: bool,
    want_chain: bool,
    keys: Option<KeyList>,
    tex: Textures,
}

impl MobileApp {
    pub fn new(home: PathBuf, cc: &egui::Context) -> MobileApp {
        let tex = Textures::load(cc);
        let mut a = MobileApp {
            home,
            tab: Tab::Files,
            entries: Vec::new(),
            sel: None,
            dlg: None,
            phrase: String::new(),
            pair_pass: String::new(),
            use_pair: false,
            gen_sel: 0,
            pair: None,
            op: None,
            msg: None,
            want_seed_dlg: false,
            want_chain: false,
            keys: None,
            tex,
        };
        a.rescan();
        a
    }

    fn rescan(&mut self) {
        self.entries.clear();
        let mut dirs: Vec<Entry> = Vec::new();
        let mut dnas: Vec<Entry> = Vec::new();
        let mut rest: Vec<Entry> = Vec::new();
        if let Ok(rd) = std::fs::read_dir(&self.home) {
            for e in rd.flatten() {
                let p = e.path();
                let Some(name) = p.file_name().map(|n| n.to_string_lossy().into_owned())
                else {
                    continue;
                };
                if name.starts_with('.') || name == "kch.db" {
                    continue;
                }
                if p.is_dir() {
                    dirs.push(Entry {
                        name,
                        is_dir: true,
                        is_dna: false,
                        meta: tr("каталог", "folder").to_string(),
                    });
                } else if name.ends_with(".dna") {
                    dnas.push(Entry {
                        name,
                        is_dir: false,
                        is_dna: true,
                        meta: fmt_size(&p),
                    });
                } else {
                    rest.push(Entry {
                        name,
                        is_dir: false,
                        is_dna: false,
                        meta: fmt_size(&p),
                    });
                }
            }
        }
        dirs.sort_by(|a, b| a.name.cmp(&b.name));
        dnas.sort_by(|a, b| a.name.cmp(&b.name));
        rest.sort_by(|a, b| a.name.cmp(&b.name));
        self.entries = dirs;
        self.entries.extend(dnas);
        self.entries.extend(rest);
        self.pair = ["keys.hkpair", "pair.hkpair"]
            .iter()
            .map(|n| self.home.join(n))
            .find(|p| p.exists());
        self.sel = None;
    }

    /* report.pdf.dna -> report.pdf: the stored name rides inside the
       archive header, the file name only carries it home */
    fn out_name(dna: &str) -> String {
        match Path::new(dna).file_stem() {
            Some(st) => st.to_string_lossy().into_owned(),
            None => format!("{dna}.out"),
        }
    }

    fn start(
        &mut self, label: &'static str,
        work: impl FnOnce() -> Result<JobOut, String> + Send + 'static,
    ) {
        let (tx, rx) = channel();
        self.op = Some(Op { label, rx });
        std::thread::spawn(move || {
            let _ = tx.send(work());
        });
    }

    fn poll(&mut self) {
        let done = match &self.op {
            Some(op) => match op.rx.try_recv() {
                Ok(r) => Some(r),
                Err(_) => None,
            },
            None => None,
        };
        let Some(out) = done else { return };
        self.op = None;
        match out {
            Ok(JobOut::Keys(l)) => {
                self.msg = Some((
                    format!("{} {:02}", i18n::s().generation, l.current_gen),
                    true,
                ));
                self.keys = Some(*l);
            }
            Ok(JobOut::Msg(m)) => {
                if self.want_seed_dlg {
                    self.dlg = Some(Dlg::Seed(m.clone()));
                }
                self.msg = Some((m, true));
            }
            Err(e) => self.msg = Some((e, false)),
        }
        self.want_seed_dlg = false;
        if self.want_chain {
            self.want_chain = false;
            self.load_chain();
        } else {
            self.rescan();
        }
    }

    fn kch(&self) -> PathBuf {
        self.home.join("kch.db")
    }

    fn load_chain(&mut self) {
        if self.phrase.split_whitespace().count() < 12 {
            return;
        }
        let phrase = self.phrase.clone();
        let kch = self.kch().to_string_lossy().into_owned();
        self.start(i18n::s().reload, move || {
            let l = wrapper::list_generations(&phrase, Some(&kch))?;
            Ok(JobOut::Keys(Box::new(l)))
        });
    }
}

fn fmt_size(p: &Path) -> String {
    match std::fs::metadata(p) {
        Ok(m) => {
            let b = m.len();
            if b >= 1024 * 1024 {
                format!("{:.1} МБ", b as f64 / (1024.0 * 1024.0))
            } else {
                format!("{} КБ", b / 1024)
            }
        }
        Err(_) => "?".to_string(),
    }
}

fn mac_btn(ui: &mut egui::Ui, label: &str, def: bool, w: f32) -> bool {
    ui.add(
        egui::Button::new(egui::RichText::new(label).font(crate::fonts::font_id(13.5, false)))
            .fill(pixel::PAPER)
            .stroke(egui::Stroke::new(if def { 2.0_f32 } else { 1.0_f32 }, pixel::INK))
            .rounding(9.0)
            .min_size(egui::vec2(w, BTN_H)),
    )
    .clicked()
}

impl eframe::App for MobileApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        self.poll();

        egui::TopBottomPanel::top("topbar").show(ctx, |ui| {
            ui.add_space(SP2);
            ui.horizontal(|ui| {
                ui.add(
                    egui::Image::new(self.tex.get("logo", Tone::Ink))
                        .fit_to_exact_size(egui::vec2(20.0, 20.0)),
                );
                ui.label(
                    egui::RichText::new("HELIX").strong().font(crate::fonts::font_id(16.0, false)),
                );
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    let gen = self.keys.as_ref().map_or(0, |k| k.current_gen);
                    ui.label(
                        egui::RichText::new(format!("{} {:02}", i18n::s().generation, gen))
                            .font(crate::fonts::font_id(12.0, true)),
                    );
                    ui.label(
                        egui::RichText::new(
                            self.op.as_ref().map_or(tr("ГОТОВ", "READY"), |o| o.label),
                        )
                        .font(crate::fonts::font_id(11.0, true)),
                    );
                });
            });
            ui.add_space(SP);
            /* the title-bar pinstripes, one row tall */
            let (rect, _) =
                ui.allocate_exact_size(egui::vec2(ui.available_width(), 4.0), egui::Sense::hover());
            let p = ui.painter_at(rect);
            let mut y = rect.top() + 0.5;
            while y < rect.bottom() {
                p.line_segment(
                    [egui::pos2(rect.left(), y), egui::pos2(rect.right(), y)],
                    egui::Stroke::new(1.0_f32, pixel::INK),
                );
                y += 3.0;
            }
        });

        egui::TopBottomPanel::bottom("actionbar").show(ctx, |ui| {
            ui.add_space(SP);
            ui.horizontal(|ui| {
                let w = (ui.available_width() - 2.0 * SP) / 3.0;
                match self.tab {
                    Tab::Files => {
                        if mac_btn(ui, i18n::s().encrypt, true, w) {
                            self.open_encrypt();
                        }
                        if mac_btn(ui, i18n::s().decrypt, false, w) {
                            self.open_decrypt();
                        }
                        if mac_btn(ui, i18n::s().delete, false, w) {
                            self.open_confirm();
                        }
                    }
                    Tab::Keys => {
                        if mac_btn(ui, i18n::s().evolve, false, w) {
                            self.evolve();
                        }
                        if mac_btn(ui, i18n::s().seed_words, true, w) {
                            self.seed();
                        }
                        if mac_btn(ui, i18n::s().reveal, false, w) {
                            self.show_key();
                        }
                    }
                }
            });
            ui.add_space(SP);
        });

        egui::CentralPanel::default()
            .frame(
                egui::Frame::none()
                    .fill(pixel::PAPER)
                    .inner_margin(egui::Margin::symmetric(SP2, SP)),
            )
            .show(ctx, |ui| {
                /* tabs: bold text, the active one underlined in accent */
                ui.horizontal(|ui| {
                    for (t, label) in [(Tab::Files, i18n::s().files), (Tab::Keys, i18n::s().keys)] {
                        let cur = self.tab == t;
                        let txt = egui::RichText::new(label).strong().font(crate::fonts::font_id(
                            if cur { 14.0 } else { 13.0 },
                            false,
                        ));
                        let resp = ui
                            .add(
                                egui::Button::new(txt)
                                    .fill(pixel::PAPER)
                                    .min_size(egui::vec2(0.0, 28.0)),
                            )
                            .on_hover_cursor(egui::CursorIcon::PointingHand);
                        let r = resp.rect;
                        if cur {
                            ui.painter().rect_filled(
                                egui::Rect::from_min_max(
                                    egui::pos2(r.left(), r.bottom() - 2.0),
                                    egui::pos2(r.right(), r.bottom()),
                                ),
                                0.0,
                                pixel::ACCENT,
                            );
                        }
                        if resp.clicked() {
                            self.tab = t;
                        }
                        ui.add_space(SP3);
                    }
                });
                ui.add_space(SP2);
                match self.tab {
                    Tab::Files => self.files_tab(ui),
                    Tab::Keys => self.keys_tab(ui),
                }
            });

        self.dialogs(ctx);
        self.toast(ctx);
    }
}

impl MobileApp {
    fn files_tab(&mut self, ui: &mut egui::Ui) {
        if self.entries.is_empty() {
            ui.add_space(SP3);
            ui.label(
                egui::RichText::new(tr(
                    "папка пуста — скопируй файлы в папку приложения",
                    "folder is empty — copy files into the app folder",
                ))
                .small(),
            );
            return;
        }
        egui::ScrollArea::vertical().show(ui, |ui| {
            for i in 0..self.entries.len() {
                let (name, is_dir, is_dna, meta) = {
                    let e = &self.entries[i];
                    (e.name.clone(), e.is_dir, e.is_dna, e.meta.clone())
                };
                let icon = if is_dir {
                    "folder"
                } else if is_dna {
                    "dna"
                } else {
                    "doc"
                };
                let sel = self.sel == Some(i);
                let resp = ui
                    .add(
                        egui::Button::new(egui::RichText::new(""))
                            .fill(if sel { pixel::INK } else { pixel::PAPER })
                            .stroke(egui::Stroke::NONE)
                            .rounding(4.0)
                            .min_size(egui::vec2(ui.available_width(), ROW_H)),
                    )
                    .on_hover_cursor(egui::CursorIcon::PointingHand);
                let r = resp.rect;
                let tone = if sel { Tone::Paper } else { Tone::Ink };
                let fg = if sel { pixel::PAPER } else { pixel::INK };
                egui::Image::new(self.tex.get(icon, tone))
                    .fit_to_exact_size(egui::vec2(24.0, 24.0))
                    .paint_at(ui, egui::Rect::from_center_size(
                        egui::pos2(r.left() + 20.0, r.center().y),
                        egui::vec2(24.0, 24.0),
                    ));
                let p = ui.painter_at(r);
                p.text(
                    egui::pos2(r.left() + 40.0, r.center().y),
                    egui::Align2::LEFT_CENTER,
                    name,
                    crate::fonts::font_id(13.5, false),
                    fg,
                );
                p.text(
                    egui::pos2(r.right() - 6.0, r.center().y),
                    egui::Align2::RIGHT_CENTER,
                    meta,
                    crate::fonts::font_id(11.0, true),
                    fg,
                );
                if resp.clicked() {
                    self.sel = if sel { None } else { Some(i) };
                }
                ui.add_space(2.0);
            }
        });
    }

    fn keys_tab(&mut self, ui: &mut egui::Ui) {
        ui.label(egui::RichText::new(i18n::s().d_mnemonic).small());
        let resp = egui::TextEdit::singleline(&mut self.phrase)
            .font(crate::fonts::font_id(13.0, true))
            .desired_width(ui.available_width())
            .hint_text(i18n::s().d_mnemonic_hint)
            .show(ui);
        if resp.response.lost_focus() && ui.ctx().input(|i| i.key_pressed(egui::Key::Enter)) {
            self.load_chain();
        }
        ui.add_space(SP);
        match &self.keys {
            None => {
                ui.label(
                    egui::RichText::new(tr(
                        "цепочка не загружена — введи фразу и нажми эволюцию",
                        "chain not loaded — enter the phrase, then evolve",
                    ))
                    .small(),
                );
            }
            Some(list) => {
                egui::ScrollArea::vertical().show(ui, |ui| {
                    for (i, k) in list.keys.iter().enumerate() {
                        let cur = i as u32 == list.current_gen;
                        let (rect, resp) = ui.allocate_exact_size(
                            egui::vec2(ui.available_width(), 36.0),
                            egui::Sense::hover(),
                        );
                        let p = ui.painter_at(rect);
                        if cur {
                            p.rect_filled(rect, 4.0, pixel::INK);
                        }
                        p.rect_filled(
                            egui::Rect::from_center_size(
                                egui::pos2(rect.left() + 12.0, rect.center().y),
                                egui::vec2(9.0, 9.0),
                            ),
                            1.0,
                            if cur { pixel::ACCENT } else { pixel::INK },
                        );
                        p.text(
                            egui::pos2(rect.left() + 26.0, rect.center().y),
                            egui::Align2::LEFT_CENTER,
                            format!("{:02}", i),
                            crate::fonts::font_id(12.0, true),
                            if cur { pixel::PAPER } else { pixel::INK },
                        );
                        p.text(
                            egui::pos2(rect.left() + 60.0, rect.center().y),
                            egui::Align2::LEFT_CENTER,
                            wrapper::hex(&k[..5]),
                            crate::fonts::font_id(12.0, true),
                            if cur { pixel::PAPER } else { pixel::INK },
                        );
                        resp.on_hover_text(tr(
                            "перезапуск с фразы: 600k pbkdf2 + 10000 шагов ≈ 3.4 с",
                            "restart from the phrase: 600k pbkdf2 + 10000 steps ≈ 3.4 s",
                        ));
                    }
                });
            }
        }
    }

    fn open_encrypt(&mut self) {
        let Some(i) = self.sel else {
            self.msg = Some((tr("выбери файл выше", "select a file above").into(), false));
            return;
        };
        let e = &self.entries[i];
        if e.is_dir || e.is_dna {
            self.msg = Some((
                tr("выбери незашифрованный файл", "pick an unencrypted file").into(),
                false,
            ));
            return;
        }
        self.dlg = Some(Dlg::Encrypt {
            path: self.home.join(&e.name).to_string_lossy().into_owned(),
        });
    }

    fn open_decrypt(&mut self) {
        let Some(i) = self.sel else {
            self.msg = Some((tr("выбери файл выше", "select a file above").into(), false));
            return;
        };
        let e = &self.entries[i];
        if e.is_dir || !e.is_dna {
            self.msg = Some((tr("выбери .dna-файл", "pick a .dna file").into(), false));
            return;
        }
        self.dlg = Some(Dlg::Decrypt {
            path: self.home.join(&e.name).to_string_lossy().into_owned(),
        });
    }

    fn open_confirm(&mut self) {
        let Some(i) = self.sel else { return };
        let e = &self.entries[i];
        if e.is_dir {
            return;
        }
        self.dlg = Some(Dlg::Confirm {
            path: self.home.join(&e.name).to_string_lossy().into_owned(),
        });
    }

    fn seed(&mut self) {
        self.want_seed_dlg = true;
        self.start(i18n::s().seed_words, || {
            wrapper::gen_seed(24).map(JobOut::Msg)
        });
    }

    fn evolve(&mut self) {
        if self.phrase.split_whitespace().count() < 12 {
            self.msg = Some((tr("сначала введи фразу", "enter the phrase first").into(), false));
            return;
        }
        let phrase = self.phrase.clone();
        let kch = self.kch().to_string_lossy().into_owned();
        self.want_chain = true;
        self.start(i18n::s().evolve, move || {
            let (gen, _) = wrapper::evolve(&phrase, Some(&kch))?;
            Ok(JobOut::Msg(format!("{} {:02}", i18n::s().generation, gen)))
        });
    }

    fn show_key(&mut self) {
        let Some(list) = &self.keys else {
            self.msg = Some((tr("цепочка не загружена", "chain not loaded").into(), false));
            return;
        };
        let i = list.current_gen as usize;
        if let Some(k) = list.keys.get(i) {
            self.dlg = Some(Dlg::Seed(wrapper::hex(k)));
        }
    }

    fn dialogs(&mut self, ctx: &egui::Context) {
        let Some(d) = self.dlg.clone() else { return };
        let mut close = false;
        let mut act: Option<Act> = None;
        let title = match d {
            Dlg::Encrypt { .. } => i18n::s().d_encrypt,
            Dlg::Decrypt { .. } => i18n::s().d_decrypt,
            Dlg::Confirm { .. } => i18n::s().d_delete,
            Dlg::Seed(_) => i18n::s().seed_words,
        };
        egui::Window::new(
            egui::RichText::new(title).strong().font(crate::fonts::font_id(14.0, false)),
        )
        .collapsible(false)
        .resizable(false)
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .frame(
            egui::Frame::none()
                .fill(pixel::PAPER)
                .stroke(egui::Stroke::new(1.0, pixel::INK))
                .rounding(10.0)
                .inner_margin(egui::Margin::symmetric(SP2, SP2)),
        )
        .show(ctx, |ui| match d {
            Dlg::Encrypt { path } => {
                ui.label(egui::RichText::new(&path).font(crate::fonts::font_id(11.0, true)));
                ui.add_space(SP);
                ui.label(egui::RichText::new(i18n::s().d_mnemonic).small());
                ui.add(
                    egui::TextEdit::singleline(&mut self.phrase)
                        .font(crate::fonts::font_id(13.0, true))
                        .desired_width(280.0)
                        .hint_text(i18n::s().d_mnemonic_hint),
                );
                ui.add_space(SP);
                ui.horizontal(|ui| {
                    ui.label(egui::RichText::new(i18n::s().d_generation).small());
                    if ui.button("−").clicked() {
                        self.gen_sel = self.gen_sel.saturating_sub(1);
                    }
                    ui.label(
                        egui::RichText::new(format!("{:02}", self.gen_sel))
                            .font(crate::fonts::font_id(13.0, true)),
                    );
                    if ui.button("+").clicked() {
                        self.gen_sel = (self.gen_sel + 1).min(999);
                    }
                });
                if self.pair.is_some() {
                    let mut up = self.use_pair;
                    ui.add(egui::Checkbox::new(
                        &mut up,
                        tr("v3: пара ключей (два фактора)", "v3: key pair (two-factor)"),
                    ));
                    if up {
                        ui.label(egui::RichText::new(tr("пароль пары", "pair passphrase")).small());
                        ui.add(
                            egui::TextEdit::singleline(&mut self.pair_pass)
                                .password(true)
                                .desired_width(280.0),
                        );
                    }
                    self.use_pair = up;
                }
                ui.add_space(SP2);
                ui.horizontal(|ui| {
                    if mac_btn(ui, i18n::s().run, true, 90.0) {
                        let pair_data = self
                            .pair
                            .clone()
                            .filter(|_| self.use_pair)
                            .map(|p| (p.to_string_lossy().into_owned(), self.pair_pass.clone()));
                        act = Some(Act::Encrypt {
                            path: path.clone(),
                            gen: self.gen_sel,
                            pair: pair_data,
                        });
                        close = true;
                    }
                    if mac_btn(ui, i18n::s().cancel, false, 90.0) {
                        close = true;
                    }
                });
            }
            Dlg::Decrypt { path } => {
                ui.label(egui::RichText::new(&path).font(crate::fonts::font_id(11.0, true)));
                ui.add_space(SP);
                ui.label(egui::RichText::new(i18n::s().d_mnemonic).small());
                ui.add(
                    egui::TextEdit::singleline(&mut self.phrase)
                        .font(crate::fonts::font_id(13.0, true))
                        .desired_width(280.0)
                        .hint_text(i18n::s().d_mnemonic_hint),
                );
                ui.add_space(SP2);
                ui.horizontal(|ui| {
                    if mac_btn(ui, i18n::s().run, true, 90.0) {
                        act = Some(Act::Decrypt { path: path.clone() });
                        close = true;
                    }
                    if mac_btn(ui, i18n::s().cancel, false, 90.0) {
                        close = true;
                    }
                });
            }
            Dlg::Confirm { path } => {
                ui.label(egui::RichText::new(i18n::s().d_delete_warn));
                ui.label(
                    egui::RichText::new(&path).font(crate::fonts::font_id(11.0, true)),
                );
                ui.add_space(SP2);
                ui.horizontal(|ui| {
                    if mac_btn(ui, i18n::s().delete, true, 90.0) {
                        act = Some(Act::Delete { path: path.clone() });
                        close = true;
                    }
                    if mac_btn(ui, i18n::s().cancel, false, 90.0) {
                        close = true;
                    }
                });
            }
            Dlg::Seed(text) => {
                ui.label(egui::RichText::new(&text).font(crate::fonts::font_id(13.0, true)));
                ui.add_space(SP2);
                ui.horizontal(|ui| {
                    if mac_btn(ui, tr("копировать", "copy"), true, 110.0) {
                        act = Some(Act::Copy(text.clone()));
                        close = true;
                    }
                    if mac_btn(ui, i18n::s().cancel, false, 90.0) {
                        close = true;
                    }
                });
            }
        });
        match act {
            Some(Act::Encrypt { path, gen, pair }) => {
                let phrase = self.phrase.clone();
                let kch = self.kch().to_string_lossy().into_owned();
                let out = Path::new(&path)
                    .parent()
                    .unwrap_or(Path::new("."))
                    .join(Self::out_name(&path));
                let out = out.to_string_lossy().into_owned();
                let phrase2 = phrase.clone();
                self.start(i18n::s().encrypt, move || {
                    let pair_ref = pair.as_ref().map(|(a, b)| (a.as_str(), b.as_str()));
                    wrapper::encrypt_ex(&path, &out, &phrase2, Some(&kch), gen, pair_ref)?;
                    Ok(JobOut::Msg(tr("готово", "done").to_string()))
                });
            }
            Some(Act::Decrypt { path }) => {
                let phrase = self.phrase.clone();
                let kch = self.kch().to_string_lossy().into_owned();
                let out = Path::new(&path)
                    .parent()
                    .unwrap_or(Path::new("."))
                    .join(Self::out_name(&path));
                let out = out.to_string_lossy().into_owned();
                self.start(i18n::s().decrypt, move || {
                    wrapper::decrypt_ex(&path, &out, &phrase, Some(&kch), None)?;
                    Ok(JobOut::Msg(tr("готово", "done").to_string()))
                });
            }
            Some(Act::Delete { path }) => match std::fs::remove_file(&path) {
                Ok(_) => {
                    self.msg = Some((tr("удалено", "deleted").into(), true));
                    self.rescan();
                }
                Err(e) => self.msg = Some((format!("! {e}"), false)),
            },
            Some(Act::Copy(t)) => {
                ctx.copy_text(t);
                self.msg = Some((tr("скопировано", "copied").into(), true));
            }
            None => {}
        }
        if close {
            self.dlg = None;
        }
    }

    fn toast(&mut self, ctx: &egui::Context) {
        let Some((m, _)) = &self.msg else { return };
        egui::Area::new(egui::Id::new("toast"))
            .anchor(egui::Align2::CENTER_BOTTOM, [0.0, -56.0])
            .order(egui::Order::Foreground)
            .show(ctx, |ui| {
                egui::Frame::none()
                    .fill(pixel::PAPER)
                    .stroke(egui::Stroke::new(1.0, pixel::INK))
                    .rounding(8.0)
                    .inner_margin(egui::Margin::symmetric(SP2, SP))
                    .show(ui, |ui| {
                        ui.label(egui::RichText::new(m).font(crate::fonts::font_id(12.0, true)));
                    });
            });
        if ctx.input(|i| i.pointer.any_click()) {
            self.msg = None;
        }
    }
}
