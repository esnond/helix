# helix v5 Design Constitution

This document is the **source of truth** for the v5 design language. All implementation decisions defer here. The HTML reference (`docs/design/v5_reference.html`) is a visual confirmation only; when a rule needs a detail, open the HTML. No literal port of HTML markup.

---

## 1. Palette

### Dark (default)
| Role | Hex | Usage |
|------|-----|-------|
| bg | `#131314` | Root background |
| bg2 | `#18181a` | Secondary surfaces |
| field | `#1d1d20` | Input fields, cards |
| ink | `#d6d3cd` | Primary text |
| ink2 | `#8d8a84` | Secondary/muted text |
| line | `#26262a` | Primary borders, dividers |
| line2 | `#333338` | Subtle borders |
| accent | `#c8a24a` | Primary actions, highlights |
| accent2 | `#8a6f33` | Accent hover/pressed |
| ok | `#7fa88f` | Success states |
| warn | `#c29a5a` | Warnings |
| err | `#c26a5a` | Errors |

### Light
| Role | Hex | Usage |
|------|-----|-------|
| bg | `#f2efe9` | Root background |
| bg2 | `#eae6dd` | Secondary surfaces |
| field | `#e5dfd4` | Input fields, cards |
| ink | `#22201d` | Primary text |
| ink2 | `#64605a` | Secondary/muted text |
| line | `#dbd5c8` | Primary borders, dividers |
| line2 | `#c9c2b2` | Subtle borders |
| accent | `#7a5c1e` | Primary actions, highlights |
| accent2 | `#8a6f33` | Accent hover/pressed |
| ok | `#3f6b52` | Success states |
| warn | `#6f5518` | Warnings |
| err | `#9c3f2e` | Errors |

### Anti-glare rules
- **Never pure black** (`#000`) for backgrounds
- **Never pure white** (`#fff`) for text
- Stroke width **≥ 1.4px** on all borders
- **Polarity = user toggle** (dark/light persisted)

---

## 2. Typography

| Role | Font | Size | Weight | Tracking | Case |
|------|------|------|--------|----------|------|
| UI text | IBM Plex Sans | 13px | Regular | 0 | Sentence case |
| Data/numbers | IBM Plex Mono | 13px | Regular | Tabular nums | — |
| Micro-heads | IBM Plex Sans | 11px | Medium | +0.5em | UPPERCASE |
| Body | IBM Plex Sans | 13px | Regular | 0 | Sentence case |
| Button/Action | IBM Plex Sans | 13px | Medium | 0 | Sentence case (task verbs) |

**Rules:**
- Sentence case for all task verbs ("Encrypt file", not "ENCRYPT FILE")
- Micro-heads: 11px uppercase, tracked +0.5em
- Tabular numerals in all data columns
- No bare glyph without label

---

## 3. Layout

| Constraint | Value |
|------------|-------|
| Max visible groups | 4 |
| Files panel | 62fr (flex) |
| Keys panel | 340px fixed |
| Separator | Hairline (1px), not gap-of-void boxes |
| Keys column | Floats lower (inset from top) |
| Actions | Menu rows with kbd hint on right |
| Status bar | Hairline with blinking block cursor + last-op journal |

---

## 4. Motion

| Parameter | Value |
|-----------|-------|
| Easing | `cubic-bezier(.2,.8,.2,1)` |
| Dialog morph | FROM source rect |
| Row birth | 320ms stagger |
| Head type-on | 140ms per char |
| Calm toggle | Kills all motion |
| `prefers-reduced-motion` | Kills all motion |
| Decorative animation | **Zero** |

---

## 5. States

| State | Requirement |
|-------|-------------|
| Empty dir | Cause + one next action |
| Read failure | Cause + one next action |
| Jobs queue | Strip with progress hairline, reason line, cancel |
| Completion | Nacre = 1px brass sweep (`accent`) |
| Dot bloom | On row completion |
| Fault | Glitch 120ms, then silence |

---

## 6. Dendrite (Generation Chain Viz)

| Rule | Spec |
|------|------|
| Nodes | Generations (not individual keys) |
| Notches | Mutations on edges |
| Current gen | Filled with `accent` |
| Row hover | Highlights its generation node |
| Old-gen rows | Carry notch; tooltip = honest re-derive cost |

---

## 7. Internationalization

- **RU / EN** full dictionary
- Toggle persisted in config
- Help as two static blocks (no generated text)

---

## 8. Sound (Physical Engine)

**Signal chain:**
```
bus → convolver(procedural metal-box IR, wet 0.13)
    → compressor
    → lowpass 14kHz
    → out ramp
```

**Sources (procedural, not samples):**
| Source | Parameters |
|--------|------------|
| Contact tick | Noise HP filter |
| Inharmonic plate | Modes 1 : 2.76 : 5.4 : 8.9; per-mode decay; jitter |
| Body thump | Delayed 6ms |
| Secondary contact | +42ms jittered |
| Line | Bandpass-noise sweep (calibration) + packet bursts (crawling with progress) |
| Coil hum | 50Hz / 53.9Hz (+100Hz / 107.8Hz harmonic for laptop speakers) while queue lives |
| Fault | Relay AM 100Hz on low saw + dull double contact |

**Seeding:** `hash(mode + name + bucket)` for jitter determinism
**Sonify flag:** Activity-envelope band breathing
**Defaults:** All OFF (opt-in only)
**Visual twin:** Every sound has a visual counterpart

---

## 9. Anti-Patterns (FORBIDDEN)

| Anti-Pattern | Forbidden |
|--------------|-----------|
| Pure black/white | ✅ Never `#000` bg, never `#fff` ink |
| Bare spinner | ✅ Use progress hairline + reason line |
| Decorative motion | ✅ Zero |
| MC symmetry | ✅ Asymmetric layouts only |
| Boxes-as-hierarchy | ✅ Hairline separators, not boxes |
| >4 visible groups | ✅ Max 4 |
| Ornament in reading layer | ✅ None |
| Sustained red | ✅ Errors flash then settle to `err` color |
| Scanlines as style | ✅ Not in v5 |
| Phi as guarantee | ✅ No golden-ratio claims |
| Naked glyph without label | ✅ Every icon has text label |
| Violet-cyan glow | ✅ Not in palette |
| Glassmorphism blur | ✅ No blur/backdrop-filter |

---

## 10. Implementation Constraints (Rust/egui)

- **Single audited FFI module** (`helix-gui/src/core/helix_wrapper.rs`) — no `unsafe` elsewhere
- **clippy -D warnings** enforced
- **Jobs queue** mirrors v5 strip (progress hairline, reason, cancel)
- **Dendrite** = egui custom paint (not HTML canvas port)
- **Sound** via `cpal` with direct-convolution IR (11ms, direct not FFT) + plate/tick/thump/relay/hum synth
- **i18n** dictionary from this constitution
- **Persistence** via config file (theme/calm/snd/sonify/lang)
- **CLI + GUI** call same core → byte-identical outputs for same inputs