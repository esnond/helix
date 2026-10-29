//! ru/en strings. one dictionary struct, two instances; the toggle is
//! persisted next to the other ui preferences.
use std::sync::atomic::{AtomicU8, Ordering};

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Lang {
    Ru,
    En,
}

pub struct Strings {
    pub files: &'static str,
    pub keys: &'static str,
    pub encrypt: &'static str,
    pub decrypt: &'static str,
    pub evolve: &'static str,
    pub reload: &'static str,
    pub export_seed: &'static str,
    pub settings: &'static str,
    pub language: &'static str,
    pub polarity: &'static str,
    pub dark: &'static str,
    pub light: &'static str,
    pub jobs: &'static str,
    pub no_jobs: &'static str,
    pub empty_dir: &'static str,
    pub read_failed: &'static str,
    pub generation: &'static str,
    pub busy: &'static str,
    pub delete: &'static str,
    pub refresh: &'static str,
    pub up: &'static str,
    pub seed_words: &'static str,
    pub hint: &'static str,
    pub help_1: &'static str,
    pub help_2: &'static str,
}

pub const RU: Strings = Strings {
    files: "файлы",
    keys: "ключи",
    encrypt: "зашифровать",
    decrypt: "расшифровать",
    evolve: "следующее поколение",
    reload: "обновить",
    export_seed: "показать фразу",
    settings: "настройки",
    language: "язык",
    polarity: "полярность",
    dark: "тёмная",
    light: "светлая",
    jobs: "задачи",
    no_jobs: "очередь пуста",
    empty_dir: "пустая папка",
    read_failed: "каталог не читается",
    generation: "поколение",
    busy: "занято",
    delete: "удалить",
    refresh: "обновить",
    up: "вверх",
    seed_words: "ввести фразу",
    hint: "клик — выбрать, двойной клик — открыть; кнопки справа сверху",
    help_1: "j/k — выбор, e — шифровать, enter — открыть, delete — удалить",
    help_2: "шифрование привязано к поколению ключа и фразе",
};

pub const EN: Strings = Strings {
    files: "files",
    keys: "keys",
    encrypt: "encrypt",
    decrypt: "decrypt",
    evolve: "next generation",
    reload: "reload",
    export_seed: "reveal phrase",
    settings: "settings",
    language: "language",
    polarity: "polarity",
    dark: "dark",
    light: "light",
    jobs: "jobs",
    no_jobs: "queue idle",
    empty_dir: "empty folder",
    read_failed: "cannot read folder",
    generation: "generation",
    busy: "busy",
    delete: "delete",
    refresh: "refresh",
    up: "up",
    seed_words: "enter phrase",
    hint: "click to select, double click to open; buttons top right",
    help_1: "j/k select, e encrypt, enter open, delete remove",
    help_2: "encryption is bound to the key generation and the phrase",
};

static LANG: AtomicU8 = AtomicU8::new(0);

pub fn set(lang: Lang) {
    LANG.store(match lang {
        Lang::Ru => 0,
        Lang::En => 1,
    }, Ordering::Relaxed);
}

pub fn lang() -> Lang {
    if LANG.load(Ordering::Relaxed) == 0 {
        Lang::Ru
    } else {
        Lang::En
    }
}

pub fn s() -> &'static Strings {
    match lang() {
        Lang::Ru => &RU,
        Lang::En => &EN,
    }
}
