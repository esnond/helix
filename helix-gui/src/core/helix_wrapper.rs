//! raw c bindings and a thin safe layer over the helix core.
//! every buffer is owned by rust; the c side never allocates.

use std::ffi::{c_char, c_int, c_uint, CString};

pub const KEY_LEN: usize = 32;

#[derive(Clone)]
pub struct KeyList {
    pub keys: Vec<[u8; KEY_LEN]>,
    pub current_gen: u32,
}

extern "C" {
    #[allow(dead_code)]
    fn helix_gui_gen_seed(
        words: c_uint, out_mnemonic: *mut c_char, out_size: usize,
        out_entropy: *mut u8, entropy_len: usize,
    ) -> c_int;
    fn helix_gui_list_generations(
        mnemonic: *const c_char, keychain_path: *const c_char,
        out_keys: *mut u8, max: c_uint, count: *mut c_uint,
        current_gen: *mut c_uint,
    ) -> c_int;
    #[allow(dead_code)]
    fn helix_gui_encrypt(
        inp: *const c_char, out: *const c_char, mnemonic: *const c_char,
        keychain_path: *const c_char, generation: c_uint,
    ) -> c_int;
    #[allow(dead_code)]
    fn helix_gui_decrypt(
        inp: *const c_char, out: *const c_char, mnemonic: *const c_char,
        keychain_path: *const c_char,
    ) -> c_int;
    fn helix_gui_encrypt_ex(
        inp: *const c_char, out: *const c_char, mnemonic: *const c_char,
        keychain_path: *const c_char, generation: c_uint,
        pair_path: *const c_char, pair_pass: *const c_char,
    ) -> c_int;
    fn helix_gui_decrypt_ex(
        inp: *const c_char, out: *const c_char, mnemonic: *const c_char,
        keychain_path: *const c_char, pair_path: *const c_char,
        pair_pass: *const c_char,
    ) -> c_int;
    fn helix_gui_check_mnemonic(mnemonic: *const c_char) -> c_int;
    fn helix_gui_pair_create(
        path: *const c_char, pass: *const c_char, argon2id: c_int,
    ) -> c_int;
    fn helix_gui_pair_export(
        path: *const c_char, pass: *const c_char, out_path: *const c_char,
    ) -> c_int;
    fn helix_gui_evolve(
        mnemonic: *const c_char, keychain_path: *const c_char,
        gen_out: *mut c_uint, key: *mut u8,
    ) -> c_int;
}

fn err(rc: c_int) -> &'static str {
    match rc {
        -1 => "io error",
        -2 => "not a valid .dna file or pair file",
        -3 => "engine failure: connectome table missing (relaunch the \
installed binary) or the keychain is unusable",
        -7 => "invalid phrase: a word is misspelled or the checksum \
does not match (12× abandon is not valid; use 11× abandon + about)",
        -4 => "authentication failed (wrong phrase or corrupt file)",
        -5 => "out of memory",
        -6 => "output file already exists",
        _ => "unknown error",
    }
}

/// generates a fresh bip39 mnemonic from the os entropy pool
#[allow(dead_code)]
pub fn gen_seed(words: u32) -> Result<String, String> {
    let mut mbuf = vec![0u8; 256];
    let mut entropy = [0u8; 32];
    let rc = unsafe {
        helix_gui_gen_seed(
            words,
            mbuf.as_mut_ptr() as *mut c_char,
            mbuf.len(),
            entropy.as_mut_ptr(),
            32,
        )
    };
    if rc != 0 {
        return Err("seed generation failed".into());
    }
    let end = mbuf.iter().position(|&b| b == 0).unwrap_or(0);
    Ok(String::from_utf8_lossy(&mbuf[..end]).into_owned())
}

/// loads every stored generation plus the current one. costs one
/// pbkdf2 (the master fingerprint needs the master key).
pub fn list_generations(
    mnemonic: &str, keychain_path: Option<&str>,
) -> Result<KeyList, String> {
    let m = CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?;
    let k = keychain_path
        .map(CString::new)
        .transpose()
        .map_err(|_| "bad path".to_string())?;
    let mut keys = vec![[0u8; KEY_LEN]; 1024];
    let mut count: c_uint = 0;
    let mut cur: c_uint = 0;
    let rc = unsafe {
        helix_gui_list_generations(
            m.as_ptr(),
            k.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            keys.as_mut_ptr() as *mut u8,
            1024,
            &mut count,
            &mut cur,
        )
    };
    if rc != 0 {
        return Err(err(rc).into());
    }
    keys.truncate(count as usize);
    Ok(KeyList { keys, current_gen: cur })
}

/// encrypts one file into the .dna format (v2; tests)
#[allow(dead_code)]
pub fn encrypt(
    inp: &str, out: &str, mnemonic: &str, keychain_path: Option<&str>,
    generation: u32,
) -> Result<(), String> {
    let (i, o, m) = (
        CString::new(inp).unwrap(),
        CString::new(out).unwrap(),
        CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?,
    );
    let k = keychain_path
        .map(CString::new)
        .transpose()
        .map_err(|_| "bad path".to_string())?;
    let rc = unsafe {
        helix_gui_encrypt(
            i.as_ptr(),
            o.as_ptr(),
            m.as_ptr(),
            k.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            generation,
        )
    };
    if rc == 0 {
        Ok(())
    } else {
        Err(err(rc).into())
    }
}

/// decrypts a .dna archive (v2; tests)
#[allow(dead_code)]
pub fn decrypt(
    inp: &str, out: &str, mnemonic: &str, keychain_path: Option<&str>,
) -> Result<(), String> {
    let (i, o, m) = (
        CString::new(inp).unwrap(),
        CString::new(out).unwrap(),
        CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?,
    );
    let k = keychain_path
        .map(CString::new)
        .transpose()
        .map_err(|_| "bad path".to_string())?;
    let rc = unsafe {
        helix_gui_decrypt(
            i.as_ptr(),
            o.as_ptr(),
            m.as_ptr(),
            k.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
        )
    };
    if rc == 0 {
        Ok(())
    } else {
        Err(err(rc).into())
    }
}

/// encrypts one file; a pair file switches to the v3 two-factor format
pub fn encrypt_ex(
    inp: &str, out: &str, mnemonic: &str, keychain_path: Option<&str>,
    generation: u32, pair: Option<(&str, &str)>,
) -> Result<(), String> {
    let (i, o, m) = (
        CString::new(inp).unwrap(),
        CString::new(out).unwrap(),
        CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?,
    );
    let k = keychain_path
        .map(CString::new)
        .transpose()
        .map_err(|_| "bad path".to_string())?;
    let (p, pp) = match pair {
        Some((path, pass)) => (
            Some(CString::new(path).map_err(|_| "bad path".to_string())?),
            Some(CString::new(pass).map_err(|_| "bad path".to_string())?),
        ),
        None => (None, None),
    };
    let rc = unsafe {
        helix_gui_encrypt_ex(
            i.as_ptr(),
            o.as_ptr(),
            m.as_ptr(),
            k.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            generation,
            p.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            pp.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
        )
    };
    if rc == 0 {
        Ok(())
    } else {
        Err(err(rc).into())
    }
}

/// decrypts a .dna archive; v3 files need the external pair file
pub fn decrypt_ex(
    inp: &str, out: &str, mnemonic: &str, keychain_path: Option<&str>,
    pair: Option<(&str, &str)>,
) -> Result<(), String> {
    let (i, o, m) = (
        CString::new(inp).unwrap(),
        CString::new(out).unwrap(),
        CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?,
    );
    let k = keychain_path
        .map(CString::new)
        .transpose()
        .map_err(|_| "bad path".to_string())?;
    let (p, pp) = match pair {
        Some((path, pass)) => (
            Some(CString::new(path).map_err(|_| "bad path".to_string())?),
            Some(CString::new(pass).map_err(|_| "bad path".to_string())?),
        ),
        None => (None, None),
    };
    let rc = unsafe {
        helix_gui_decrypt_ex(
            i.as_ptr(),
            o.as_ptr(),
            m.as_ptr(),
            k.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            p.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            pp.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
        )
    };
    if rc == 0 {
        Ok(())
    } else {
        Err(err(rc).into())
    }
}

/// bip39 wordlist+checksum validation, no derivation cost. the check
/// has its own error domain: -1 here means «invalid phrase», not the
/// dna io error that shares the number.
pub fn phrase_valid(mnemonic: &str) -> Result<(), String> {
    let m = CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?;
    let rc = unsafe { helix_gui_check_mnemonic(m.as_ptr()) };
    if rc == 0 {
        Ok(())
    } else {
        Err("invalid phrase: words must come from the bip39 english \
             list and the checksum must match — 12× abandon is NOT a \
             valid phrase, use «gen-seed» or 11× abandon + about"
            .to_string())
    }
}

/// creates an external kyber keypair file (.hkpair)
pub fn pair_create(path: &str, pass: &str, argon2id: bool) -> Result<(), String> {
    let (p, pp) = (
        CString::new(path).map_err(|_| "bad path".to_string())?,
        CString::new(pass).map_err(|_| "bad passphrase".to_string())?,
    );
    let rc = unsafe { helix_gui_pair_create(p.as_ptr(), pp.as_ptr(), argon2id as i32) };
    if rc == 0 {
        Ok(())
    } else {
        Err(err(rc).into())
    }
}

/// writes the raw public key of a pair file
pub fn pair_export(path: &str, pass: &str, out: &str) -> Result<(), String> {
    let (p, pp, o) = (
        CString::new(path).map_err(|_| "bad path".to_string())?,
        CString::new(pass).map_err(|_| "bad passphrase".to_string())?,
        CString::new(out).map_err(|_| "bad path".to_string())?,
    );
    let rc = unsafe { helix_gui_pair_export(p.as_ptr(), pp.as_ptr(), o.as_ptr()) };
    if rc == 0 {
        Ok(())
    } else {
        Err(err(rc).into())
    }
}

/// advances the evolution chain one generation
pub fn evolve(
    mnemonic: &str, keychain_path: Option<&str>,
) -> Result<(u32, [u8; KEY_LEN]), String> {
    let m = CString::new(mnemonic).map_err(|_| "bad mnemonic".to_string())?;
    let k = keychain_path
        .map(CString::new)
        .transpose()
        .map_err(|_| "bad path".to_string())?;
    let mut gen: c_uint = 0;
    let mut key = [0u8; KEY_LEN];
    let rc = unsafe {
        helix_gui_evolve(
            m.as_ptr(),
            k.as_ref().map_or(std::ptr::null(), |p| p.as_ptr()),
            &mut gen,
            key.as_mut_ptr(),
        )
    };
    if rc != 0 {
        return Err(err(rc).into());
    }
    Ok((gen, key))
}

/// rust side helper mirroring the c header: hex encode for display
pub fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    const MNEMONIC: &str = "abandon abandon abandon abandon abandon \
        abandon abandon abandon abandon abandon abandon about";

    #[test]
    fn gen_seed_has_right_word_count() {
        let m = gen_seed(24).unwrap();
        assert_eq!(m.split_whitespace().count(), 24);
        assert_eq!(gen_seed(12).unwrap().split_whitespace().count(), 12);
    }

    #[test]
    fn dna_roundtrip_headless() {
        let _g = engine_guard::ENGINE.lock().unwrap();
        // cargo test runs from helix-gui/, the engine wants the
        // vendored connectome one level up
        std::env::set_var(
            "HELIX_CONNECTOME",
            "../vendor/petri/data/herm_302.csv",
        );
        let kch = format!(
            "/tmp/hx_rt_{:?}/kch.db",
            std::process::id()
        );
        let dir = std::env::temp_dir();
        let inp = dir.join("helix_gui_test.in");
        let dna = dir.join("helix_gui_test.dna");
        let out = dir.join("helix_gui_test.out");
        for f in [&inp, &dna, &out] {
            let _ = std::fs::remove_file(f);
        }
        std::fs::write(&inp, b"gui roundtrip payload").unwrap();
        encrypt(
            inp.to_str().unwrap(),
            dna.to_str().unwrap(),
            MNEMONIC,
            Some(&kch),
            3,
        )
        .unwrap();
        decrypt(
            dna.to_str().unwrap(),
            out.to_str().unwrap(),
            MNEMONIC,
            Some(&kch),
        )
        .unwrap();
        assert_eq!(
            std::fs::read(&inp).unwrap(),
            std::fs::read(&out).unwrap()
        );
        // wrong mnemonic must fail with the auth error
        assert!(decrypt(
            dna.to_str().unwrap(),
            out.to_str().unwrap(),
            &gen_seed(12).unwrap(),
            Some(&kch)
        )
        .is_err());
        let _ = std::fs::remove_file(inp);
        let _ = std::fs::remove_file(dna);
        let _ = std::fs::remove_file(out);
    }
}

#[cfg(test)]
mod engine_guard {
    /* the c engine keeps derived state in statics and is not thread
       safe (limitations 9); the gui serializes jobs through the queue,
       tests serialize through this lock. */
    pub static ENGINE: std::sync::Mutex<()> = std::sync::Mutex::new(());
}

#[cfg(test)]
mod gui_flow_tests {
    /* headless functional sweep: every gui action exercised through the
       same wrapper calls the ui makes.HELIX_CONNECTOME is pinned to the
       build tree so the tests run from any cwd. */
    use super::*;

    const MNEMONIC: &str = "abandon abandon abandon abandon abandon \
        abandon abandon abandon abandon abandon abandon about";
    const CONNECTOME: &str =
        "/home/esnond/helix/vendor/petri/data/herm_302.csv";

    fn setup(tag: &str) {
        std::env::set_var("HELIX_CONNECTOME", CONNECTOME);
        let _ = std::fs::create_dir_all(format!("/tmp/hx_{tag}"));
    }

    fn p(tag: &str, name: &str) -> String {
        format!("/tmp/hx_{tag}/{name}")
    }

    fn rm(path: &str) {
        let _ = std::fs::remove_file(path);
    }

    fn put(path: &str, what: &[u8]) {
        std::fs::write(path, what).unwrap();
    }

    fn same(a: &str, b: &str) -> bool {
        std::fs::read(a).unwrap() == std::fs::read(b).unwrap()
    }

    #[test]
    fn full_flow() {
        let _g = engine_guard::ENGINE.lock().unwrap();
        setup("flow");
        let phrase = p("flow", "in.bin");
        let v2 = p("flow", "in.bin.dna");
        let v2out = p("flow", "in.bin.out");
        let pair = p("flow", "pair.hkpair");
        let kch = p("flow", "kch.db");
        let pubk = p("flow", "pair.pub");
        let v3 = p("flow", "v3.dna");
        let v3out = p("flow", "v3.out");
        for f in [&phrase, &v2, &v2out, &pair, &pubk, &v3, &v3out, &kch] {
            rm(f);
        }
        put(&phrase, b"gui functional sweep payload");

        /* gen-seed: word count for both lengths */
        assert_eq!(gen_seed(12).unwrap().split_whitespace().count(), 12);
        assert_eq!(gen_seed(24).unwrap().split_whitespace().count(), 24);

        /* phrase validation: good passes, a typo is caught before any
           job would start */
        assert!(phrase_valid(MNEMONIC).is_ok());
        assert!(phrase_valid("abandon abandon WRONG abandon abandon             abandon abandon abandon abandon abandon abandon about")
        .is_err());
        /* all-abandon fails the checksum: the classic user trap */
        assert!(phrase_valid(
            "abandon abandon abandon abandon abandon abandon abandon \
             abandon abandon abandon abandon abandon"
        )
        .is_err());

        /* evolve: chain advances, generations increase; the keychain
           is a private test file — tests must never touch $HOME */
        let (g1, _k1) = evolve(MNEMONIC, Some(&kch)).unwrap();
        let (g2, _k2) = evolve(MNEMONIC, Some(&kch)).unwrap();
        assert_eq!(g2, g1 + 1);

        /* list: the keychain caches everything up to current */
        let list = list_generations(MNEMONIC, Some(&kch)).unwrap();
        assert!(list.keys.len() >= g2 as usize);

        /* encrypt v2 (phrase only), decrypt, bitwise equal */
        encrypt_ex(&phrase, &v2, MNEMONIC, None, 0, None).unwrap();
        decrypt_ex(&v2, &v2out, MNEMONIC, None, None).unwrap();
        assert!(same(&phrase, &v2out));

        /* wrong phrase: authentication failure, not a crash */
        assert!(decrypt_ex(&v2, &v2out, &gen_seed(12).unwrap(), None, None)
            .unwrap_err()
            .contains("authentication"));

        /* bad phrase at encrypt: -7 surfaces as the -7 text; fresh
           target because the overwrite refusal fires first by design */
        let bad = p("flow", "bad.dna");
        rm(&bad);
        let e = encrypt_ex(
            &phrase,
            &bad,
            "abandon abandon WRONG abandon abandon abandon abandon \
             abandon abandon abandon abandon about",
            None,
            0,
            None,
        )
        .unwrap_err();
        assert!(e.contains("invalid phrase"), "{e}");

        /* the gui names outputs stem.dna (report.pdf -> report.dna) */
        assert_eq!(
            std::path::Path::new("/x/report.pdf")
                .file_stem()
                .map(|s| format!("{}.dna", s.to_string_lossy())),
            Some("report.dna".to_string())
        );

        /* overwrite refusal before any state change */
        assert!(encrypt_ex(&phrase, &v2, MNEMONIC, None, 0, None)
            .unwrap_err()
            .contains("already exists"));

        /* pair: create, export, v3 two-factor roundtrip */
        pair_create(&pair, "sweep pass", true).unwrap();
        pair_export(&pair, "sweep pass", &pubk).unwrap();
        assert!(std::path::Path::new(&pubk).exists());
        encrypt_ex(&phrase, &v3, MNEMONIC, None, 0,
                   Some((&pair, "sweep pass"))).unwrap();
        decrypt_ex(&v3, &v3out, MNEMONIC, None,
                   Some((&pair, "sweep pass"))).unwrap();
        assert!(same(&phrase, &v3out));

        /* v3 without the pair: two-factor refusal */
        assert!(decrypt_ex(&v3, &v3out, MNEMONIC, None, None)
            .unwrap_err()
            .contains("authentication"));
        /* v3 with a wrong pair passphrase */
        assert!(decrypt_ex(&v3, &v3out, MNEMONIC, None,
                           Some((&pair, "wrong")))
            .unwrap_err()
            .contains("authentication"));

        for f in [&phrase, &v2, &v2out, &pair, &pubk, &v3, &v3out, &bad, &kch]
        {
            rm(f);
        }
        let _ = std::fs::remove_dir("/tmp/hx_flow");
    }
}
