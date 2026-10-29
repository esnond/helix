# r4' decision — helixd is cancelled (HX-8)

Date: 2026-10-29. Decided under the HX-7 owner decision (helix is a
self-contained file encryptor; `.dna` decrypts anywhere from the mnemonic
alone; the "directory service" r4 is cancelled).

## Options considered

1. **helixd — an Erlang/OTP batch daemon**: a long-lived node that watches
   a drop directory, encrypts/decrypts batches, keeps an audit log, and
   exposes a control port.
2. **Documented cancellation**: no daemon in r3-r5; the CLI and the GUI
   are the whole product.

## Decision: option 2

Reasons, in order of weight:

- **No consumer.** HX-7 removed the directory service, which was the only
  thing that needed a resident process. Nothing in the current product
  wants a daemon: one file at a time, one mnemonic, no shared state.
- **Attack surface for nothing.** A listening OTP node would add a network
  protocol, a supervision tree, a release packaging story and a second
  language's worth of review surface, in exchange for batch convenience
  that a shell loop already provides (`for f in *.tar; do helix encrypt
  ...; done`).
- **The seam rule.** The Proteus pattern (sim -> artefact -> control plane)
  does not apply here: there is no simulator artefact to hand over. Adding
  a control plane would be architecture cosplay, so it is refused on the
  same grounds the emergent-behavior gate refuses goal-directed code.
- **Verification cost.** Everything in r5 is checked by `make test` and
  `make test-san`. A daemon would need a supervision test harness that does
  not exist yet, and shipping an untested long-running service is exactly
  the kind of overclaim R7 forbids.

## What is kept instead

- `scripts/push_run.sh` and the existing CLI cover batch use.
- If a real need appears (multi-user batch, audit trail), the daemon is
  revisited as r6 with its own test harness, not bolted on now.

## Falsifier

Reopen this decision if a concrete user workflow appears that needs
(a) a resident process for correctness rather than convenience, or
(b) an audit trail that cannot be produced by appending CLI output.
