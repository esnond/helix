//! settings strip: polarity, language, and the two motion switches the
//! constitution allows. the pbkdf2 iteration count stays a compile time
//! constant (suckless config policy) so it is shown, not a slider.
//! preferences persist to $XDG_CONFIG_HOME/helix/gui.conf, one key=value
//! per line, so the toggle survives a restart.

use eframe::egui::{self, RichText};
use std::path::PathBuf;

use crate::i18n::{self, Lang};
use crate::style::colors;

#[derive(Clone, Copy)]
pub struct Settings {
    /// motion: the constitution's calm switch. when false every animated
    /// effect is suppressed.
    pub motion: bool,
    /// honour the desktop's reduced-motion hint
    pub prefers_reduced: bool,
    pub glow: f32,
    pub light: bool,
    pub lang: Lang,
}

impl Default for Settings {
    fn default() -> Self {
        Settings {
            motion: true,
            prefers_reduced: false,
            glow: 1.0,
            light: false,
            lang: Lang::Ru,
        }
    }
}

fn config_path() -> Option<PathBuf> {
    let base = std::env::var("XDG_CONFIG_HOME")
        .ok()
        .filter(|s| !s.is_empty())
        .map(PathBuf::from)
        .or_else(|| std::env::var("HOME").ok().map(|h| PathBuf::from(h).join(".config")))?;
    Some(base.join("helix").join("gui.conf"))
}

impl Settings {
    pub fn load() -> Settings {
        let mut s = Settings::default();
        let Some(path) = config_path() else {
            return s;
        };
        let Ok(text) = std::fs::read_to_string(path) else {
            return s;
        };
        for line in text.lines() {
            let Some((k, v)) = line.split_once('=') else {
                continue;
            };
            match k.trim() {
                "motion" => s.motion = v.trim() == "1",
                "glow" => s.glow = v.trim().parse().unwrap_or(1.0),
                "light" => s.light = v.trim() == "1",
                "lang" => {
                    s.lang = if v.trim() == "en" { Lang::En } else { Lang::Ru };
                }
                _ => {}
            }
        }
        s
    }

    pub fn save(&self) {
        let Some(path) = config_path() else {
            return;
        };
        if let Some(dir) = path.parent() {
            let _ = std::fs::create_dir_all(dir);
        }
        let text = format!(
            "motion={}\nglow={}\nlight={}\nlang={}\n",
            if self.motion { 1 } else { 0 },
            self.glow,
            if self.light { 1 } else { 0 },
            match self.lang {
                Lang::Ru => "ru",
                Lang::En => "en",
            }
        );
        let _ = std::fs::write(path, text);
    }

    pub fn apply(&self) {
        colors::set_light(self.light);
        i18n::set(self.lang);
    }
}

pub fn show(ui: &mut egui::Ui, s: &mut Settings) -> bool {
    let mut changed = false;
    let t = colors::theme();
    let s_txt = i18n::s();
    ui.horizontal(|ui| {
        ui.label(crate::style::fonts::micro_head(s_txt.settings));
        ui.add_space(16.0);

        ui.label(RichText::new(s_txt.polarity).color(t.ink2).small());
        if ui
            .selectable_label(!s.light, RichText::new(s_txt.dark).color(t.ink))
            .clicked()
            && s.light
        {
            s.light = false;
            changed = true;
        }
        if ui
            .selectable_label(s.light, RichText::new(s_txt.light).color(t.ink))
            .clicked()
            && !s.light
        {
            s.light = true;
            changed = true;
        }

        ui.add_space(16.0);
        ui.label(RichText::new(s_txt.language).color(t.ink2).small());
        if ui
            .selectable_label(s.lang == Lang::Ru, RichText::new("ru").color(t.ink))
            .clicked()
        {
            s.lang = Lang::Ru;
            changed = true;
        }
        if ui
            .selectable_label(s.lang == Lang::En, RichText::new("en").color(t.ink))
            .clicked()
        {
            s.lang = Lang::En;
            changed = true;
        }

        ui.add_space(16.0);
        if ui
            .checkbox(&mut s.motion, RichText::new("motion").color(t.ink))
            .changed()
        {
            changed = true;
        }
        ui.label(
            RichText::new("pbkdf2 600k iters (compile time)")
                .color(t.ink2)
                .small(),
        );
    });
    if changed {
        s.apply();
        s.save();
    }
    changed
}

/// the micro-head helper for a strip title
pub fn head(text: &str) -> RichText {
    crate::style::fonts::micro_head(text)
}
