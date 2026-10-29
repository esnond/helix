//! persisted preferences: $XDG_CONFIG_HOME/helix/gui.conf, key=value.
//! window size and last directory included, so a launcher start lands
//! where the user left off.

use crate::i18n::Lang;
use std::path::PathBuf;

#[derive(Clone)]
pub struct Prefs {
    pub light: bool,
    pub sound: bool,
    pub lang: Lang,
    pub win: (f32, f32),
    pub last_dir: Option<String>,
}

impl Default for Prefs {
    fn default() -> Self {
        Prefs {
            light: false,
            sound: false, /* constitution: every sound off by default */
            lang: Lang::Ru,
            win: (1040.0, 660.0),
            last_dir: None,
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

impl Prefs {
    pub fn load() -> Prefs {
        let mut p = Prefs::default();
        let Some(path) = config_path() else { return p };
        let Ok(text) = std::fs::read_to_string(path) else { return p };
        for line in text.lines() {
            let Some((k, v)) = line.split_once('=') else { continue };
            let v = v.trim();
            match k.trim() {
                "light" => p.light = v == "1",
                "sound" => p.sound = v == "1",
                "lang" => p.lang = if v == "en" { Lang::En } else { Lang::Ru },
                "win_w" => p.win.0 = v.parse().unwrap_or(p.win.0),
                "win_h" => p.win.1 = v.parse().unwrap_or(p.win.1),
                "last_dir" if !v.is_empty() => p.last_dir = Some(v.to_string()),
                _ => {}
            }
        }
        p
    }

    pub fn save(&self) {
        let Some(path) = config_path() else { return };
        if let Some(dir) = path.parent() {
            let _ = std::fs::create_dir_all(dir);
        }
        let text = format!(
            "light={}\nsound={}\nlang={}\nwin_w={:.0}\nwin_h={:.0}\nlast_dir={}\n",
            i32::from(self.light),
            i32::from(self.sound),
            match self.lang {
                Lang::Ru => "ru",
                Lang::En => "en",
            },
            self.win.0,
            self.win.1,
            self.last_dir.as_deref().unwrap_or(""),
        );
        let _ = std::fs::write(path, text);
    }

    pub fn apply(&self) {
        crate::theme::set_light(self.light);
        crate::i18n::set(self.lang);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /* the file the app writes must roundtrip through load */
    #[test]
    fn prefs_roundtrip_text() {
        let p = Prefs {
            light: true,
            sound: true,
            lang: Lang::En,
            win: (800.0, 600.0),
            last_dir: Some("/tmp/helix test".into()),
        };
        let text = format!(
            "light={}\nsound={}\nlang={}\nwin_w={:.0}\nwin_h={:.0}\nlast_dir={}\n",
            i32::from(p.light),
            i32::from(p.sound),
            "en",
            p.win.0,
            p.win.1,
            p.last_dir.as_deref().unwrap()
        );
        let mut q = Prefs::default();
        for line in text.lines() {
            let Some((k, v)) = line.split_once('=') else { continue };
            match k {
                "light" => q.light = v == "1",
                "sound" => q.sound = v == "1",
                "lang" => q.lang = Lang::En,
                "win_w" => q.win.0 = v.parse().unwrap(),
                "win_h" => q.win.1 = v.parse().unwrap(),
                "last_dir" => q.last_dir = Some(v.into()),
                _ => {}
            }
        }
        assert!(q.light && q.sound);
        assert_eq!(q.win, p.win);
        assert_eq!(q.last_dir, p.last_dir);
    }
}
