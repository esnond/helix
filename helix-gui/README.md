# helix-gui

dwm-style tiling gui for the helix key manager: sun microsystems
purple palette, serial experiments lain accents, keyboard first.

> the same warning as the main readme applies: research prototype,
> not production cryptography. this gui is a visual interface over
> the same unaudited c core.

## run

    cd helix-gui
    cargo run --release        # wayland native or x11/xwayland
    cargo test                 # headless core tests (no display needed)

the binary embeds the c core at build time (build.rs compiles the
vendored engine and the crypto modules with the same flags as the c
gate). the connectome path is resolved via the HELIX_CONNECTOME env
var when running from a subdirectory, automatic in main and tests.

## layout (dwm style)

    +----------------------------+------------+
    | files (central)            | keys (right|
    |  name  size  gen  mtime    |  tile)     |
    |  ...                       |  gen N     |
    |                            |  evolve,   |
    |                            |  export    |
    +----------------------------+------------+
    | settings strip (scanlines, glow, core info)|
    +--------------------------------------------+
    | bar: cwd        helix-gui v       gen  kch |
    +--------------------------------------------+

## keyboard

    j / k    move selection down / up
    Enter    open: decrypt .dna files, encrypt plain files
    e        encrypt selected
    d        decrypt selected .dna
    Del      delete selected (confirmation dialog)

## actions

encrypt and decrypt open a lain style terminal dialog asking for the
seed words; the c core runs on a worker thread (pbkdf2 600k plus
10000 connectome steps, a few seconds) while the ui keeps painting.
decryption needs only the mnemonic - the generation key is recomputed
from the evolution chain, the keychain is a cache. export seed prints
the current mnemonic to the status area.

## honest notes

- the mnemonic stays in process memory for the session (needed by
  every action); it is never written to disk by the gui
- the pbkdf2 iteration count is a compile time constant (see
  src/helix_config.h); the settings strip shows it instead of a slider
- crt curvature is not implemented (needs a shader pass egui does not
  expose); scanlines and focus glow are
- screenshots: this repo has no screenshot tooling installed; on a
  wlroots compositor `grim` or any screencopy client captures it
- wayland is the native path; x11 works through xwayland
