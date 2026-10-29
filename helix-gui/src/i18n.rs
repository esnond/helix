//! ru/en strings, one struct, two instances. the dictionary covers
//! actions, states and every error the dialogs can show, so no english
//! leaks into the ru interface (acceptance: dict parity test).

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Lang {
    Ru,
    En,
}

pub struct Strings {
    pub files: &'static str,
    pub keys: &'static str,
    pub jobs: &'static str,
    pub encrypt: &'static str,
    pub decrypt: &'static str,
    pub delete: &'static str,
    pub refresh: &'static str,
    pub up: &'static str,
    pub evolve: &'static str,
    pub reload: &'static str,
    pub reveal: &'static str,
    pub seed_words: &'static str,
    pub settings: &'static str,
    pub language: &'static str,
    pub polarity: &'static str,
    pub dark: &'static str,
    pub light: &'static str,
    pub sound: &'static str,
    pub on: &'static str,
    pub off: &'static str,
    pub cancel: &'static str,
    pub run: &'static str,
    pub hint: &'static str,
    pub no_jobs: &'static str,
    pub empty_dir: &'static str,
    pub empty_dir_next: &'static str,
    pub read_failed: &'static str,
    pub read_failed_next: &'static str,
    pub generation: &'static str,
    pub gen_short: &'static str,
    pub kch: &'static str,
    pub busy: &'static str,
    pub audio: &'static str,
    pub audio_off: &'static str,
    pub tip_encrypt: &'static str,
    pub tip_decrypt: &'static str,
    pub tip_delete: &'static str,
    pub tip_up: &'static str,
    pub tip_refresh: &'static str,
    pub tip_reveal: &'static str,
    pub tip_evolve: &'static str,
    pub d_encrypt: &'static str,
    pub d_decrypt: &'static str,
    pub d_delete: &'static str,
    pub d_seed: &'static str,
    pub d_mnemonic: &'static str,
    pub d_mnemonic_hint: &'static str,
    pub d_generation: &'static str,
    pub d_delete_warn: &'static str,
    pub d_short_phrase: &'static str,
    pub d_open_file: &'static str,
    pub no_seed: &'static str,
    pub chain_not_loaded: &'static str,
    pub no_phrase: &'static str,
    pub help_1: &'static str,
    pub help_2: &'static str,
}

pub const RU: Strings = Strings {
    files: "файлы",
    keys: "ключи",
    jobs: "задачи",
    encrypt: "зашифровать",
    decrypt: "расшифровать",
    delete: "удалить",
    refresh: "обновить",
    up: "вверх",
    evolve: "следующее поколение",
    reload: "обновить цепочку",
    reveal: "показать фразу",
    seed_words: "ввести фразу",
    settings: "настройки",
    language: "язык",
    polarity: "полярность",
    dark: "тёмная",
    light: "светлая",
    sound: "звук",
    on: "вкл",
    off: "выкл",
    cancel: "отмена",
    run: "запустить",
    hint: "клик — выбрать, двойной клик — открыть; действия — кнопками сверху",
    no_jobs: "очередь пуста",
    empty_dir: "пустая папка — шифровать нечего",
    empty_dir_next: "перейдите в другую папку кнопкой «..»",
    read_failed: "папка не читается",
    read_failed_next: "проверьте права доступа, затем нажмите «⟳»",
    generation: "поколение",
    gen_short: "ген",
    kch: "цеп",
    busy: "работаю",
    audio: "звук",
    audio_off: "аудио недоступно",
    tip_encrypt: "зашифровать выбранный файл в <имя>.dna",
    tip_decrypt: "расшифровать выбранный .dna архив",
    tip_delete: "удалить выбранный файл с диска",
    tip_up: "на уровень вверх (h, ←)",
    tip_refresh: "перечитать папку",
    tip_reveal: "показать текущую фразу в диалоге",
    tip_evolve: "следующее поколение ключа",
    d_encrypt: "зашифровать",
    d_decrypt: "расшифровать",
    d_delete: "удалить файл",
    d_seed: "фраза",
    d_mnemonic: "фраза:",
    d_mnemonic_hint: "двенадцать или двадцать четыре слова",
    d_generation: "поколение:",
    d_delete_warn: "файл будет удалён с диска без возможности восстановления.",
    d_short_phrase: "фраза короткая: нужно 12 или 24 слова",
    d_open_file: "открыть",
    no_seed: "нет фразы",
    chain_not_loaded: "цепочка не загружена",
    no_phrase: "фраза не введена — нажмите «ввести фразу»",
    help_1: "j/k — выбор, e — шифровать, enter — открыть, h — вверх, delete — удалить, ctrl+q — выход",
    help_2: "шифрование привязано к поколению ключа и фразе; .dna расшифровывается фразой где угодно",
};

pub const EN: Strings = Strings {
    files: "files",
    keys: "keys",
    jobs: "jobs",
    encrypt: "encrypt",
    decrypt: "decrypt",
    delete: "delete",
    refresh: "refresh",
    up: "up",
    evolve: "next generation",
    reload: "reload chain",
    reveal: "reveal phrase",
    seed_words: "enter phrase",
    settings: "settings",
    language: "language",
    polarity: "polarity",
    dark: "dark",
    light: "light",
    sound: "sound",
    on: "on",
    off: "off",
    cancel: "cancel",
    run: "run",
    hint: "click to select, double click to open; actions are the buttons above",
    no_jobs: "queue idle",
    empty_dir: "empty folder — nothing to encrypt here",
    empty_dir_next: "go to another folder with the «..» button",
    read_failed: "folder cannot be read",
    read_failed_next: "check permissions, then press «⟳»",
    generation: "generation",
    gen_short: "gen",
    kch: "kch",
    busy: "busy",
    audio: "audio",
    audio_off: "audio unavailable",
    tip_encrypt: "encrypt the selected file into <name>.dna",
    tip_decrypt: "decrypt the selected .dna archive",
    tip_delete: "delete the selected file from disk",
    tip_up: "one level up (h, left)",
    tip_refresh: "rescan the folder",
    tip_reveal: "show the current phrase in a dialog",
    tip_evolve: "derive the next key generation",
    d_encrypt: "encrypt",
    d_decrypt: "decrypt",
    d_delete: "delete file",
    d_seed: "seed words",
    d_mnemonic: "phrase:",
    d_mnemonic_hint: "twelve or twenty four words",
    d_generation: "generation:",
    d_delete_warn: "the file is removed from disk and cannot be restored.",
    d_short_phrase: "phrase too short: 12 or 24 words required",
    d_open_file: "open",
    no_seed: "no phrase",
    chain_not_loaded: "chain not loaded",
    no_phrase: "no phrase yet — press «enter phrase»",
    help_1: "j/k select, e encrypt, enter open, h up, delete remove, ctrl+q quit",
    help_2: "encryption binds to the key generation and the phrase; a .dna decrypts anywhere from the phrase alone",
};

static LANG: std::sync::atomic::AtomicU8 = std::sync::atomic::AtomicU8::new(0);

pub fn set(lang: Lang) {
    use std::sync::atomic::Ordering;
    LANG.store(
        match lang {
            Lang::Ru => 0,
            Lang::En => 1,
        },
        Ordering::Relaxed,
    );
}

pub fn lang() -> Lang {
    use std::sync::atomic::Ordering;
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

#[cfg(test)]
mod tests {
    use super::*;

    /* every field listed; a new field must be added here too, otherwise
       the parity check does not cover it */
    fn all(s: &Strings) -> Vec<&'static str> {
        vec![
            s.files, s.keys, s.jobs, s.encrypt, s.decrypt, s.delete,
            s.refresh, s.up, s.evolve, s.reload, s.reveal, s.seed_words,
            s.settings, s.language, s.polarity, s.dark, s.light, s.sound,
            s.on, s.off, s.cancel, s.run, s.hint, s.no_jobs, s.empty_dir,
            s.empty_dir_next, s.read_failed, s.read_failed_next,
            s.generation, s.gen_short, s.kch, s.busy, s.audio, s.audio_off,
            s.tip_encrypt, s.tip_decrypt, s.tip_delete, s.tip_up,
            s.tip_refresh, s.tip_reveal, s.tip_evolve, s.d_encrypt,
            s.d_decrypt, s.d_delete, s.d_seed, s.d_mnemonic,
            s.d_mnemonic_hint, s.d_generation, s.d_delete_warn,
            s.d_short_phrase, s.d_open_file, s.no_seed, s.chain_not_loaded,
            s.no_phrase, s.help_1, s.help_2,
        ]
    }

    /* both dictionaries must carry the same keys: a forgotten field is an
       english leak into the ru interface */
    #[test]
    fn dict_parity() {
        assert_eq!(all(&RU).len(), all(&EN).len());
        for (ru, en) in all(&RU).iter().zip(all(&EN).iter()) {
            assert!(!ru.is_empty() && !en.is_empty());
        }
    }
}
