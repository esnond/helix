# r1 security analysis: key evolution

written from the inline adversary and methodologist passes (subagent
spawns were blocked provider-side on 2026-10-28; review performed in
session and recorded in the decision log as HX-4).

## what the evolution actually is

the chain is a deterministic expansion of the master key:
every gen_n is a fixed function of (master_key, n). no entropy is
created after the master key exists; entropy is distributed across
generations. the recombination narrative does not add entropy, it
shapes how the 512 bits of master key material project onto 256-bit
generation values.

## claims, precisely

1. compromise of gen_n alone reveals nothing about gen_{n-1} or
   gen_{n+1}: each is hidden behind an independent 256-bit mask
   sha256(mk || n) with ~2^256 preimage work. tested mechanically in
   tests/test_forward_secrecy.c (mask always applied, dictionary of
   master key candidates never predicts the next generation).
2. given the master key, all generations past and future are
   computable. this is identical to r0 and is the stated limitation of
   the whole design: the master key is the root of trust.
3. given the keychain WITHOUT the master key: all past generations are
   readable (they are stored in plaintext), and past masks are
   algebraically recoverable (mask_n = gen_n ^ gen_{n-1} ^ gen_{n-2}),
   but future generations remain hidden behind future masks.
4. wrong master key material (one byte off) produces output at
   hamming ~78 from the truth - correlated only through the shared
   xor base, still requiring the unknown mask.

## what forward secrecy means here, honestly

the brief claims "compromise of gen_n doesn't reveal gen_n-1". true,
but the operationally meaningful statement is weaker:

- the keychain contains every past key in plaintext. anyone with the
  keychain has every past generation. forward secrecy in the standard
  sense (past keys stay secret after a long-term secret compromise)
  does NOT hold against keychain compromise, because there is nothing
  to reveal: the keys are on disk.
- what holds: past generations are not derivable from a single leaked
  generation, and future generations are not predictable from the full
  keychain without the master key.
- protecting the keychain file (0600) is therefore as critical as
  protecting the mnemonic. losing the mnemonic AND the keychain loses
  everything; losing only the mnemonic loses future generations after
  the highest stored one and nothing before it that is still on disk.

## assumptions

- sha256 remains preimage resistant (~2^256 mask hiding)
- the master key is not compromised (same root assumption as r0)
- the keychain file is protected at the OS level (0600, single user)
- no side channel leaks the master key during pbkdf2 (r2+ work:
  memlock considerations are out of scope for r1)

## limitations

- quantum: sha256 preimage is Grover-bounded (~2^128), the masks hold
  post quantum; the pbkdf2 cost drops for a quantum attacker; r3 adds
  the real post quantum layer.
- the keychain is append-only but not tamper-evident: an attacker with
  write access can truncate it (losing generations) or overwrite
  entries in place. r2 should add a hash chain over entries; noted as
  an r2 requirement.
- single-user, single-process: no file locking; concurrent evolvers on
  one keychain race (documented, not defended in r1).
