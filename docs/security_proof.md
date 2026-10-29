# security_proof — the chain argument, stated so it can be checked

Status: **research prototype**. Nothing here is peer reviewed or
mechanized. The document exists so that the claim "a leaked generation
does not reveal the previous one" is a checkable statement instead of a
slogan. Every assumption is named, and the reduction loss is written out.
Registered as h1.2 in `docs/hypotheses.md`, re-stated after the
methodologist rejected the first version (AGENT_LOG A1).

## 1. What is not claimed

Not claimed: that sha256 is a PRF (assumption), that the implementation is
audited (it is not), that the keychain file protects anything (it does
not: it stores every generation in clear), or that the argument covers
side channels (it does not).

## 2. The objects

- `mk` — 64-byte master key, `PBKDF2-HMAC-SHA512(entropy, salt, 600000)`.
  Entropy is 128 bits for a 12-word BIP39 mnemonic, 256 bits for 24 words.
  `mk` has 512 bits of length but at most `H(entropy)` bits of entropy, so
  every bound below is capped at `2^-128` (12 words) or `2^-256` (24).
- `g0` — the engine phase: `sha256` over the quantized trajectory of the
  448-cell connectome driven by `mk`. Treated as `g0 = F0(mk)`.
- Chain: for `n >= 1`,
  `g_n = g_{n-1} XOR g_{n-2} XOR D_n(mk)`,
  where `D_n(mk)` has exactly `k_n` one-bits, `k_n = 32 + (P_0 mod 33)`,
  and `P_n = sha256(mk || u32be(n) || "helix-mut")` is the mask source
  (src/evolve.c). The bit positions are selected by walking the bytes of
  `P_n` and the iterator is branchless with a fixed 512-draw trip count.

## 3. Assumption (the only cryptographic one)

**PRF-1.** `P_n = sha256(mk || u32be(n) || "helix-mut")` is a
pseudorandom function of `n` under key `mk`: no adversary that cannot
guess `mk` distinguishes `P_n` from a uniform 256-bit string with
advantage above `eps_PRF`.

Note what is *not* assumed: preimage resistance of an unobserved hash
digest. A leaked `g_n` does not expose `sha256(mk || n)`; the mask is
never revealed, and the previous first version of this document claimed a
reduction to a preimage that does not exist. That was wrong.

## 4. The game

`G(eps)`:
1. A challenger samples `mk` from the mnemonic entropy.
2. The adversary A receives `(g_{n}, g_{n+1}, ..., g_{n+T})` for adaptively
   chosen `n` and `T` — **consecutive** generations only, `T <= q`.
3. A outputs a guess `y` for `g_{n-1}`.
4. A wins if `y = g_{n-1}`.

The keychain file and any single generation outside the window are
**excluded** from the game. With the keychain, `g_{n-1}` is stored in
clear and no argument is needed.

## 5. Reduction

Given A with advantage `eps > 0` in `G`, build a PRF distinguisher B with
advantage at least `eps / q`:

1. B is handed an oracle that returns either `P_n = PRF(mk, n)` or a
   uniform string, for each `n` it asks.
2. B simulates the challenger: it samples `g_0, g_1` uniformly (the
   distribution of the first two generations is not needed — A only sees
   the window, and the window is generated from the oracle).
3. For each step in the window, B derives `D_n` from the oracle answer,
   computes `g_n = g_{n-1} XOR g_{n-2} XOR D_n`, and feeds A the window.
4. A outputs `y`; B checks whether `y` is consistent with the simulated
   chain. A consistent `y` means the oracle's answer was used correctly
   and A's recovery succeeded; over a uniform oracle, consistency has
   probability `2^-256` per candidate (the mask has full support).

`A` needs one oracle answer per generation in its window, so `q` answers
are consumed; by a standard hybrid argument the distinguishing advantage
degrades to `eps / q`. Hence

```
Adv_recover(G)  <=  q * Adv_PRF(sha256) + 2^-128      (12 words)
                <=  q * Adv_PRF(sha256) + 2^-256      (24 words)
```

The `2^-128` term is the brute-force ceiling over the mnemonic entropy; no
chain argument can lift it.

## 6. Consequence that is often misstated

With **two adjacent generations and the mask**, the previous generation
falls out: `g_{n-1} = g_{n+1} XOR g_n XOR D_{n+1}`. The whole security of
the chain is therefore the secrecy of `D_{n+1}`, i.e. of the PRF output
without `mk`. Any text claiming "even with the whole chain the past is
hidden" is false; the correct statement is: with the chain and without
`mk`, recovering any missing generation costs approximately one PRF
distinguishing game, and the reduction above is the precise form of that.

## 7. What a reviewer should check

- `src/evolve.c` computes `D_n` from `mk`, `n` and the domain tag only, and
  `mk` never enters the output except through `sha256`.
- `src/kdf.c:helix_generation_key` recomputes the chain from the mnemonic
  and never writes to the keychain.
- `tests/test_forward_secrecy.c` is an exploratory dictionary test, not the
  reduction; it is reported as such in `docs/hypotheses.md`.
- `tests/test_evolution.c` pins chain determinism and the mask weight.

## 8. Falsifiers

The claim is false if any of these is exhibited:
1. A map `g_n -> g_{n-1}` with success above `2^-128` that does not break
   PRF-1.
2. A distinguisher for `P_n` that needs fewer than `2^128` work on 12-word
   mnemonics (this breaks the assumption, not the reduction).
3. A generation `n` for which the mask weight is 0 or 256, making the mask
   guessable. By construction `k_n = 32 + (P_0 mod 33)` lies in `[32,64]`,
   so this cannot happen; `tests/test_evolution.c` checks the observed
   flip count over 1000 generations (mean near 48).
