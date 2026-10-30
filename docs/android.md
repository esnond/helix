# helix-android: 1-bit mac on your phone

version: 1.0.0 (shared with desktop helix-gui)
status: release candidate, design frozen to mockup «mac refined» (logo A)

## what you get

the exact same helix engine (connectome → pbkdf2 600k → 10k steps → .dna v2/v3)
wrapped in a touch-first ui that is a 1:1 copy of macintosh system 6:

- 1-bit only: black & white + a single phosphor accent (#047b62, 4.87:1 on white)
- pixel icons (kare rules: 16×16, one metaphor each: doc / dna / folder / key / trash)
- pinstripe title bar, inverted row selection, mac-style rounded buttons
- two tabs bottom: ФАЙЛЫ / КЛЮЧИ (bold text, accent underline)
- bottom action bar: encrypt / decrypt / delete  |  evolve / seed / reveal
- modal dialogs with the same fields as desktop: mnemonic, generation spinner,
  optional v3 key pair (argon2id), pair passphrase
- toasts instead of status bar; sound is off by default (can be enabled in prefs)

## build & install

```bash
# prereqs on the build machine
sudo apt install android-sdk-platform-tools
export ANDROID_HOME=/home/esnond/Android/Sdk
sdkmanager "ndk;25.2.9519653" "build-tools;33.0.2" "platforms;android-33"
rustup target add aarch64-linux-android
cargo install cargo-apk

# build
cd ~/helix
./scripts/build_android.sh
```

output: `target/release/apk/helix.apk` (unsigned, release, aarch64)

```bash
# install on device
adb install -r target/release/apk/helix.apk
```

## where files live

on android the app uses its **external files dir** (no SAF picker needed):

```
/storage/emulated/0/Android/data/ru.esnond.helix/files/
```

- copy files there with adb, mtp, or any file manager:
  `adb push report.pdf /storage/emulated/0/Android/data/ru.esnond.helix/files/`
- the connectome (herm_302.csv) is unpacked automatically on first run
- keychain lives at `kch.db` next to your files (same format as desktop KCH2)
- .dna archives write beside the source: `report.pdf` → `report.pdf.dna`

## how to use

### first run
1. open the app → you see **ФАЙЛЫ** (empty) and **КЛЮЧИ** tabs.
2. copy any file into the folder above (adb/mtp).
3. switch to **ФАЙЛЫ** tab, pull to refresh — the file appears.
4. tap the file → row inverts (black), bottom bar lights up.

### encrypt (v2, phrase only)
1. tap **ЗАШИФРОВАТЬ** (default button, double-ring).
2. enter your 12/24-word mnemonic (pt sans, tabular numbers).
3. generation spinner: 0..999 (default 0). tap **ВЫПОЛНИТЬ**.
3. done toast → `filename.dna` appears in the list (tag v2).

### encrypt (v3, two-factor)
1. put a `keys.hkpair` file in the folder (create on desktop: `helix pair create -k argon2id`).
2. tap **ЗАШИФРОВАТЬ**, enter mnemonic.
3. checkbox **v3: пара ключей (два фактора)** appears → tick it.
4. enter the pair passphrase (separate from the mnemonic).
5. **ВЫПОЛНИТЬ** → `filename.dna` tagged v3. decryption requires **both** the mnemonic AND the .hkpair file.

### decrypt
1. tap a `.dna` file → row inverts.
2. tap **РАСШИФРОВАТЬ**.
3. enter mnemonic (and pair passphrase if v3).
4. **ВЫПОЛНИТЬ** → original filename appears without `.dna` (e.g. `report.pdf`).

### delete
1. tap any file, tap **УДАЛИТЬ** → confirmation dialog with the filename.
2. confirm → toast «удалено», list refreshes.

### keys tab
1. enter mnemonic in the field, press **Enter** (or tap **ЭВОЛЮЦИЯ**).
2. the generation chain loads: dots 00 01 02… current filled in phosphor accent.
3. tap **ЭВОЛЮЦИЯ** → appends next generation (≈3.4s on modern phones).
4. tap **ПОКАЗАТЬ** → modal with the raw 32-byte hex key (copy button).
5. **СЛОВА ФРАЗЫ** generates a fresh 24-word mnemonic (opens seed dialog, copy to clipboard).

### sound / language / polarity
stored in prefs; not exposed in the mobile toolbar yet (desktop has the toggles).
to enable sound: edit `prefs.json` in the app folder (or wait for v1.1 settings screen).

## limitations (honest)

- no SAF picker → files must be in the app's external files dir.
- no drag-and-drop, multi-select, or share sheet (roadmap v1.1).
- sound engine is present but off by default (android audio policy).
- connectome ships inside the apk (unpacks on first run); 302 neurons × 10k steps ≈ 3.4s/gen on snapdragon 8 gen 1, ~8s on mid-range.
- .hkpair file must be in the same folder; no keychain manager UI yet.
- keychain migration KCH1→KCH2 works on first evolve.
- no biometric unlock (the mnemonic IS the unlock).

## desktop vs mobile

same rust library (`helix_gui`), two entry points:
- `helix-gui/src/main.rs` → eframe desktop (egui + wayland/x11)
- `helix-android/src/main.rs` → android `ndk-glue` + eframe glow

both call `MobileApp::new(home, &cc.egui_ctx)` — the ui code is **identical**.

## verify build

```bash
# run the lib tests (they exercise the c core through the wrapper)
cd ~/helix/helix-gui && cargo test --lib

# run the desktop binary for quick iteration
cd ~/helix/helix-gui && cargo run --release -- --fullscreen
```

## source map

```
helix-android/
├── Cargo.toml
├── src/main.rs          # entry, JNI home dir, connectome unpack
helix-gui/
├── src/lib.rs           # library root
├── src/mobile.rs        # the whole touch UI (this file)
├── src/pixel.rs         # 1-bit icon textures (doc/dna/folder/key/trash/logo)
├── src/wrapper.rs       # FFI → c core (helix_engine_phase, etc.)
├── src/fonts.rs         # pt sans + jetbrains mono (bundled ttf in release)
└── src/i18n.rs          # ru/en strings
```

## license

same as helix: MIT (see root LICENSE). PT Sans (OFL), JetBrains Mono (OFL), icon maps are code (CC0-equivalent).

---

generated from docs/design/r7_design_spec.md and the approved mockup «mac refined».
any design change = new mockup → owner approval → port. no freestyle.