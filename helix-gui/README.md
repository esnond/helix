# helix-gui

dwm-tiling, Serial Experiments Lain + Sun Microsystems purple aesthetic GUI for helix key manager.

## visual spec

**palette** — teal on void, purple glow, no white, no blue
- `BG_DEEP`  `#07070f` (void)
- `BG_PANEL` `#0d0d1a` (panel)
- `TEXT_MAIN` `#5eead4` (phosphor teal)
- `ACCENT` `#a855f7` (sun purple)
- `ACCENT_GLOW` `#c084fc` (purple glow)
- `WARN` `#f472b6` (magenta)
- `OK` `#34d399` (emerald)
- `BORDER` `#1e1e3a` (muted purple)

**fonts** — monospace only, regular weight, no bold
- heading: 18px
- body/mono: 13px
- small: 11px
- letter-spacing: 0.5px
- system monospace (Fira Code / JetBrains Mono / DejaVu Sans Mono)

**layout** — asymmetric 62/38 split
- left: file manager (62%, dwm tile)
- right: key manager (38%, inset 24px top margin)
- bottom: settings strip (64px)
- bottom: status bar (24px, blinking block cursor)

**effects** (all CPU, no shaders, single overlay pass where possible)
- scanlines: 1px every 3px at 12% alpha
- vignette: 8 concentric rects, 0% center → 35% edges
- noise grain: monochrome static 3% alpha, animated 12fps deterministic
- crt flicker: global brightness ±1% at 0.5Hz sine
- cursor blink: block cursor 1s on / 1s off
- text print: types label char-by-char over 180ms on focus change
- glitch: 120ms horizontal slice displacement + rgb split on error
- glow focus: 4-layer expanding strokes at 40% alpha
- selection: left 2px solid accent bar + 18% bg alpha
- prompt buttons: `> label` format, no bg fill, hover = accent glow

**perf budget** — 60fps at 1920x1080, effects layer < 2ms/frame

## run

```bash
cd helix-gui
cargo run --release        # wayland native or x11/xwayland
cargo test                 # headless core tests (no display needed)
```

### environment

- `HELIX_CONNECTOME` — override connectome csv path (default: `../vendor/petri/data/herm_302.csv`)

### keyboard

- `j` / `k` — navigate file list
- `enter` — encrypt (non-.dna) / decrypt (.dna)
- `e` — encrypt selected
- `del` — delete selected
- focus changes trigger text print effect (`> files` / `> keys`)

### settings (bottom strip)

- scanlines / vignette / noise grain / crt flicker toggles
- glow strength slider (0.0–2.0)
- pbkdf2 iterations shown (compile-time constant, 600k)

## architecture

- Rust/eframe 0.23 + egui 0.23 frontend
- C core (petri engine vendored from ~/elegans v1.0.1) via FFI shim (`gui_api.h/c`)
- `build.rs` compiles C core with `cc` crate using `-std=c99 -Wall -Wextra -pedantic -Werror -O2 -ffp-contract=off`
- deterministic: xorshift64* RNG from C core, bitwise reproducible
- worker threads for crypto operations (encrypt/decrypt/evolve/list) keep UI responsive

## files

```
helix-gui/
├── src/
│   ├── main.rs                    # app state, job system, asymmetric layout, effects layer
│   ├── core/helix_wrapper.rs      # FFI bindings
│   ├── style/
│   │   ├── mod.rs                 # apply() — palette, fonts, widget styles
│   │   ├── colors.rs              # palette constants & functions
│   │   ├── effects.rs             # scanlines, vignette, noise, glitch, glow, blink, print
│   │   └── fonts.rs               # font config
│   └── ui/
│       ├── file_manager.rs        # left tile (62%), selection with left accent bar
│       ├── key_manager.rs         # right tile (38%), gen display, prompt buttons
│       ├── encryption_dialog.rs   # lain terminal modal, glitch on error
│       ├── settings.rs            # effect toggles, glow slider
│       └── status_bar.rs          # dwm bar, blinking cursor
├── gui_api.h / gui_api.c          # C shim (thin wrappers over existing C APIs)
├── build.rs                       # cc build of vendored petri + crypto + dna + gui_api
└── Cargo.toml
```

## tests

```bash
cargo test
```

runs headless FFI tests:
- `gen_seed_has_right_word_count` — BIP39 word count validation
- `dna_roundtrip_headless` — encrypt/decrypt roundtrip via C core

## determinism

- single seed via CLI or env controls entire simulation
- no `rand()`, no `time()` in sim path
- CSV output bitwise identical across compiler configs (gcc/clang, -O0/-O2/-O3)

## license

MIT