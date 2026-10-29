# r5 spec — gui in the v5 design language

Status: research prototype front end. `eframe`/`egui` 0.23 over the same C
core the CLI uses, through one FFI shim (`helix-gui/src/gui_api.c`). No
crypto or format logic lives in Rust.

## Shipped

- **v5 palette, both polarities.** `style/colors.rs` holds the dark and
  light token sets verbatim from `docs/design/constitution.md`. Anti-glare
  rules are enforced in the tokens themselves: the darkest background is
  `#131314`, the lightest ink `#d6d3cd`, and every stroke goes through
  `colors::HAIRLINE` (1.4 px). Polarity is a persisted toggle.
- **Layout.** Files take the remaining width, the keys column is a fixed
  340 px (capped at 45 % on narrow windows), separators are hairlines.
- **Type.** IBM Plex Sans/Mono when installed; the build machine has
  neither, so it falls back to DejaVu Sans / DejaVu Sans Mono. Data (paths,
  keys, generations) is always monospace; measured 11 px micro-heads are
  the only uppercase text.
- **Jobs strip.** A 26 px strip above the settings row: micro-head, either
  "queue idle" or the current operation with a progress hairline. No bare
  spinner anywhere; the busy state in the keys column is a reason line plus
  a hairline.
- **Status bar.** cwd, blinking block cursor, the last operation as a
  one-line journal, generation and keychain size on the right.
- **Dendrite.** The generation chain is drawn, not listed: one node per
  generation, the current generation the only filled node, and notches
  encoding drift from the parent (`hamming/8`, capped at 6). Hovering a row
  highlights it. Honest note: the notch count is derived from the keys, not
  from a stored mutation log, so it shows drift, not the literal number of
  flipped mask bits.
- **i18n.** RU/EN dictionary (`src/i18n.rs`) covering the file actions,
  keys column, settings and status words; the choice persists with the
  polarity.
- **Motion.** No scanlines, no vignette, no CRT flicker — the constitution
  lists all three as anti-patterns. Only the cursor blink and an explicit
  glitch survive, and both stop when the motion switch is off.

## Deferred, and said out loud

- **Sound.** Nothing is shipped. The constitution's signal chain (convolver,
  inharmonic plate, coil hum) is not implemented; a `cpal` dependency was
  not added. Consequences: no audio bug, and no audio benefit. The rule
  "all sound off by default" is satisfied trivially.
- **Dialog morph and row birth motion.** egui 0.23 has no cubic-bezier
  easing on rects; the morph-from-source treatment is not implemented.
- **Full i18n.** Dialog bodies and help text are still English only.
- **Nacre sweep on completion.** Not drawn.

## Build

```
cd helix-gui && cargo build      # debug
cargo run                        # needs a display
cargo test                       # 2 tests, both exercise the FFI core
```

`build.rs` compiles the C core with the same flags as the root Makefile,
including the vendored math and kyber. `cargo:rustc-link-lib=m` was
removed: the GUI binary must not import libm either, same h1.1 rule.

```
