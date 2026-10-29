# helix-gui

dwm-tiling, Serial Experiments Lain + Sun Microsystems purple aesthetic GUI for helix key manager.

## design language: v5

the full rules live in `../docs/design/constitution.md`; this file only
lists what the code actually does.

**palette** — two polarities, persisted. dark: bg `#131314`, bg2 `#18181a`,
field `#1d1d20`, ink `#d6d3cd`, ink2 `#8d8a84`, line `#26262a`, accent
`#c8a24a`, ok `#7fa88f`, err `#c26a5a`. light: bg `#f2efe9`, ink `#22201d`,
accent `#7a5c1e`, err `#9c3f2e`. never pure black, never pure white,
strokes at 1.4 px minimum.

**type** — IBM Plex Sans (ui) and IBM Plex Mono (data) when installed,
DejaVu fallback otherwise. body 13 px, micro-heads 11 px uppercase,
heading 18 px. data is always monospace.

**layout** — files take the remaining width; the keys column is 340 px
(45 % cap). hairlines separate groups, four groups maximum: files, keys,
jobs, status.

**states** — empty folder and read failure both state the cause and one
next action; the jobs strip shows queued work with a progress hairline; a
busy keys column shows a reason line, not a spinner.

**dendrite** — one node per generation, notches for drift from the parent,
the current generation the only filled node, row hover highlights it.

**i18n** — RU/EN dictionary, toggle persisted next to polarity and motion.

**not shipped** — the sound engine, dialog morph animation, nacre sweep and
the full i18n of dialog bodies are deferred; `../docs/r5_spec.md` lists them.

## build and run

```
cd helix && make            # c core, needed once for the ffi surface
cd helix-gui && cargo build
cargo run                   # needs a display
cargo test                  # 2 tests, both drive the c core through ffi
```

## architecture

`src/gui_api.c` is the only unsafe boundary: it exposes the C core as
plain C functions, and `src/core/helix_wrapper.rs` is the only module that
declares them. everything above that is safe Rust. crypto jobs run on a
worker thread and report through an `mpsc` channel, so the ui never blocks
on a 600k-iteration pbkdf2.
