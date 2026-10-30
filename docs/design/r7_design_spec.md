# r7 design spec: «операторская» (laboratory / NAVI)

status: proposal for owner approval. base: mockup variant 3 (dark workman),
rebuilt from research. not yet ported to egui.

## research findings (checked, with sources)

1. golden ratio as a beauty law is a myth: fechner's rectangle experiments
   were methodologically flawed; modern replications find no preference peak
   at 1.618 (pmc 2024 "the golden ratio—dispelling the myth"; plus.maths.org).
   used here only as a harmonic tool: column split 1 : 1.618 and a spacing
   ladder 4 · 6.5 · 10.5 · 17 · 27.5 px. no aesthetic claims made.
2. phosphor reference (terminal heritage): P1 green peak 528 nm ≈ #41ff00
   (harsh), practical soft green ≈ #7fff7f; P3 amber ≈ #ffb000 / #daa520
   (superuser, wikipedia monochrome monitor). the accents in this spec are
   phosphor-derived but tonally calibrated in OKLCH, not copied raw.
3. susan kare (macintosh 1984): 32×32 grid, "meaningful, memorable, clear",
   friendliness for non-technical users, bold low-res clarity (chicago).
   taken: one metaphor per icon, instant readability, warm chrome, inverted
   (cream-on-dark) primary buttons instead of colored fills.
4. NAVI (serial experiments lain) is literally macintosh/copland derived:
   beige hardware, teal UI, dark room, machine reads text aloud (lain.wiki,
   namu.wiki). taken: teal channel accent, quiet machine voice, dark-room
   staging. rejected: glitch filters, scanlines, crt curvature — cringe, not
   craft.
5. steins;gate lab language: instrument labels, channel numbers, amber CRT,
   hand-built devices. taken: CH·01/02/03 engraved labels, LEDs, mono data.
6. color engineering: palette built in OKLCH (perceptually uniform, hue held
   constant on ramps; dark themes boost chroma, invert lightness ramps —
   evilmartians oklch guide). converted to srgb hex below.
7. contrast: WCAG 2.2 — text ≥ 4.5:1, non-text UI components ≥ 3:1 against
   adjacent colors. measured pairs (script in session log): ink/bg2 12.9:1,
   ink2/bg2 5.5:1, teal/bg2 9.6:1, amber/bg2 8.7:1, red/bg2 4.9:1 — all pass.
   ink3 (3.1:1) is decorative-only (wcag exempts inactive/decorative).
8. sound: brief ui sounds are earcons (structured abstract motifs,
   blattner et al.) or auditory icons (mimic real objects, sonification
   handbook ch.13); material design "applying sound to ui" maps one motif
   per action class. kept soft set (owner choice 2026-10-29): contact click
   1.5k+3.2k Hz / 30 ms (auditory icon of a switch), success chime E6+B5
   staggered, dud 210→150 Hz. no hum/relay/sweep (rejected as scary).

## palette tokens (oklch → srgb)

| token | oklch | hex | role |
|---|---|---|---|
| bg0 | 0.145 0.008 90 | #0b0a07 | room behind the window |
| bg1 | 0.185 0.010 90 | #14130e | window surface |
| bg2 | 0.225 0.012 90 | #1e1c15 | panels |
| bg3 | 0.270 0.014 90 | #29261f | raised / hover |
| line | 0.310 0.012 90 | #33302a | hairline |
| line2 | 0.400 0.015 90 | #4b473f | strong hairline / button border |
| ink | 0.905 0.015 95 | #e3e0d5 | primary text (aged paper) |
| ink2 | 0.660 0.015 95 | #959288 | secondary text |
| ink3 | 0.500 0.012 95 | #65635c | decorative only |
| teal | 0.800 0.115 172 | #64d6b5 | CH1 accent: keys, active, ok |
| teal_dim | 0.640 0.085 172 | #509d86 | dim accent |
| amber | 0.790 0.130 80 | #e6b14e | CH2 accent: run, attention |
| red | 0.650 0.150 25 | #dc655f | errors only |

two-channel discipline: teal = idle/ok/key state, amber = machine busy or
needs attention. never a third accent except red for errors.

## type

- labels/ui: PT Sans (cyrillic-first, ParaType; OFL-compatible) 13.5 px
- data/mono: JetBrains Mono (OFL, cyrillic) 11–12.5 px, tabular numerals
- section labels: caps 10.5 px, letter-spacing 0.09em, ink2
- both embed as TTF in egui (license permits).

## layout

- column split files : (queue+chain) = 1 : 1.618; keys rail fixed 300px.
- spacing ladder: 4 · 6.5 · 10.5 · 17 · 27.5 · 44.5 (φ steps on a 4px base).
- 1 px hairlines everywhere; radius 2 px; no gradients; no shadows inside
  the machine except one room shadow behind the window.
- disabled = 50% dither (kare heritage), not opacity.

## iconography

- lucide 16 px, 1.75 px stroke, one metaphor each: dna for .dna archives,
  file-text, folder, shield for pair files, key-round, activity for jobs.
- icons never carry meaning alone: always paired with a text label or
  tooltip (wcag 1.4.11 + kare universality).

## motion

- one pulse: amber run-dot and masthead led, 1.5 s, disabled under
  prefers-reduced-motion. no other animation. progress = hairline, never a
  spinner (house rule).

## sound map (earcon classes)

| event | sound | class |
|---|---|---|
| select/navigate | soft_click | auditory icon (switch contact) |
| job done | chime E6+B5 | earcon, confirm |
| rejected/auth fail | dud 210→150 | earcon, deny |
| job started | soft_click (lower gain) | auditory icon |

master gain 0.5, default sound off (constitution §8).
