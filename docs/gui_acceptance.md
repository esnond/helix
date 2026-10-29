# gui acceptance checklist — v1.0

Every item is checked by hand on the real display. The reviewer is the
owner; the checklist is the acceptance gate for the tag `v1.0-rc`.
Build: `cd helix-gui && cargo build --release`, run:
`./target/release/helix-gui` (or `--fullscreen`).

## Files panel (mouse only)

| # | item | ok |
|---|---|---|
| 1 | rows list folders first, folders carry a trailing `/` | |
| 2 | single click selects a row (left brass bar + hover surface) | |
| 3 | double click on a folder enters it | |
| 4 | double click on a `.dna` opens the decrypt dialog | |
| 5 | double click on a plain file opens the encrypt dialog | |
| 6 | `..` button goes one level up | |
| 7 | `⟳` re-reads the folder (create a file outside, press, see it) | |
| 8 | encrypt button disabled for folders and `.dna`, enabled for other files | |
| 9 | decrypt button disabled unless a `.dna` is selected | |
| 10 | empty folder shows the cause and the `..` next action | |
| 11 | unreadable folder shows the cause and the `⟳` next action | |

## Keys column

| # | item | ok |
|---|---|---|
| 12 | "enter phrase" opens the seed dialog; 12 words load the chain | |
| 13 | dendrite: one node per generation, current filled, notches vary | |
| 14 | hovering a chain row highlights it and shows the re-derive cost | |
| 15 | "next generation" appends a node and the strip logs it | |
| 16 | "reveal phrase" shows the phrase in a dialog | |

## Jobs strip

| # | item | ok |
|---|---|---|
| 17 | a running job shows a moving hairline and a cancel button | |
| 18 | cancel marks the row and the strip returns to idle (the running c call still completes, up to ~3.4 s — the side effect is not interruptible) | |
| 19 | success draws the 1 px brass sweep across the file panel | |
| 20 | failure shows `!` with the core error text, no spinner anywhere | |

## Settings

| # | item | ok |
|---|---|---|
| 21 | polarity toggle switches dark/light, survives a restart | |
| 22 | language toggle switches every visible string, survives restart | |
| 23 | sound toggle persists; with audio feature off the bar says unavailable | |
| 24 | ctrl+q quits; window close saves the last directory | |

## Sound (only with `--features audio` and libasound2-dev)

| # | item | ok |
|---|---|---|
| 25 | job start: contact tick + thump, line sweep while running | |
| 26 | job success: plate strike, inharmonic modes audible | |
| 27 | failure: relay AM fault sound | |
| 28 | hum starts while the queue is alive and stops after | |
| 29 | every sound has its visual twin (nacre sweep, `!`, strip activity) | |

## v3 external pair (stage 4)

| # | item | ok |
|---|---|---|
| 30 | keys column shows the "external pair" section with a path field | |
| 31 | "create pair…" opens the dialog: path, passphrase field, argon2id/pbkdf2 toggle; empty fields show an error, not a crash | |
| 32 | "export pk" writes `<pair>.pub` and the status line confirms it | |
| 33 | encrypt dialog offers "v2: phrase" / "v3: pair file"; v3 mode shows pair path + passphrase fields and remembers the path for the next dialog | |
| 34 | encrypting twice to the same target shows the core "output file already exists" error in the dialog | |
| 35 | a v3 archive decrypts with phrase + pair; with the phrase only it shows the authentication error | |

Anything unchecked at tag time is listed in `docs/r5_spec.md` as open, not
silently dropped.
