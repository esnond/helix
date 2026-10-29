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
        -3 => "generation unavailable",
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
        // cargo test runs from helix-gui/, the engine wants the
        // vendored connectome one level up
        std::env::set_var(
            "HELIX_CONNECTOME",
            "../vendor/petri/data/herm_302.csv",
        );
        let dir = std::env::temp_dir();
        let inp = dir.join("helix_gui_test.in");
        let dna = dir.join("helix_gui_test.dna");
        let out = dir.join("helix_gui_test.out");
        std::fs::write(&inp, b"gui roundtrip payload").unwrap();
        encrypt(
            inp.to_str().unwrap(),
            dna.to_str().unwrap(),
            MNEMONIC,
            None,
            3,
        )
        .unwrap();
        decrypt(
            dna.to_str().unwrap(),
            out.to_str().unwrap(),
            MNEMONIC,
            None,
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
            None
        )
        .is_err());
        let _ = std::fs::remove_file(inp);
        let _ = std::fs::remove_file(dna);
        let _ = std::fs::remove_file(out);
    }
}
