# helix: biologically inspired key manager.
# c99, posix subset, no external libraries. libm for the vendored engine.

CC       = cc
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Werror -O2 -ffp-contract=off
CPPFLAGS = -D_DEFAULT_SOURCE -Ivendor/petri -Isrc -Isrc/vendored_math
# no -lm: the derivation path carries its own math (src/vendored_math, h1.1)
LDLIBS   =
LIBM_FOR_TOOLS = -lm   # test harnesses only, never the shipped binary

VENDOR_SRC = sim connectome neuron body world modulators plasticity \
             metrics rng gcamp mutants evolution
VENDOR_OBJ = $(addprefix vendor/petri/,$(addsuffix .o,$(VENDOR_SRC)))
CRYPTO_SRC = hex sha256 sha512 hmac pbkdf2 bip39 entropy kdf keychain \
             evolve keycheck chacha20 poly1305 aead dna
CRYPTO_OBJ = $(addprefix src/,$(addsuffix .o,$(CRYPTO_SRC)))
MATH_OBJ   = src/vendored_math/vmath.o
OBJ        = $(CRYPTO_OBJ) $(MATH_OBJ)
TESTS      = tests/test_vectors tests/test_bip39 tests/test_kdf \
             tests/test_keychain tests/test_evolution \
             tests/test_backward_compat tests/test_forward_secrecy \
             tests/test_evolve tests/test_chacha tests/test_dna

all: libpetri.a $(TESTS) helix

helix: src/main.o $(OBJ) libpetri.a
	$(CC) $(CFLAGS) -o $@ src/main.o $(OBJ) libpetri.a $(LDLIBS)

vendor/petri/%.o: vendor/petri/%.c $(wildcard vendor/petri/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

libpetri.a: $(VENDOR_OBJ)
	ar rcs $@ $(VENDOR_OBJ)

src/vendored_math/%.o: src/vendored_math/%.c $(wildcard src/vendored_math/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

src/%.o: src/%.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tests/test_%: tests/test_%.o $(OBJ) libpetri.a
	$(CC) $(CFLAGS) -o $@ $< $(OBJ) libpetri.a $(LDLIBS)

tests/test_%.o: tests/test_%.c $(wildcard src/*.h) $(wildcard tests/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -c -o $@ $<

test: libpetri.a $(TESTS) helix
	sh tests/run_tests.sh

tools/evolve_chain: tools/evolve_chain.o $(OBJ) libpetri.a
	$(CC) $(CFLAGS) -o $@ $< $(OBJ) libpetri.a $(LDLIBS)

tools/evolve_chain.o: tools/evolve_chain.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tools/ct_test: tools/ct_test.o $(CRYPTO_OBJ) $(MATH_OBJ) libpetri.a
	$(CC) $(CFLAGS) -o $@ $< $(CRYPTO_OBJ) $(MATH_OBJ) libpetri.a $(LIBM_FOR_TOOLS)

tools/ct_test.o: tools/ct_test.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tools/diff_crypto: tools/diff_crypto.o $(CRYPTO_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(CRYPTO_OBJ) $(MATH_OBJ) libpetri.a

tools/diff_crypto.o: tools/diff_crypto.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tools/math_diff: tools/math_diff.o $(MATH_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(MATH_OBJ) $(LIBM_FOR_TOOLS)

tools/math_diff.o: tools/math_diff.c $(wildcard src/vendored_math/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tools/timing_bench: tools/timing_bench.o $(OBJ) libpetri.a
	$(CC) $(CFLAGS) -o $@ $< $(OBJ) libpetri.a $(LDLIBS)

tools/timing_bench.o: tools/timing_bench.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f vendor/petri/*.o src/*.o src/vendored_math/*.o libpetri.a $(TESTS) tests/*.o helix \
	      tools/evolve_chain tools/evolve_chain.o \
	      tools/math_diff tools/math_diff.o \
	      tools/diff_crypto tools/diff_crypto.o \
	      tools/ct_test tools/ct_test.o \
	      tools/timing_bench tools/timing_bench.o

# determinism-gate: the full suite must exit 0 under ubsan and asan.
# LD_PRELOAD is cleared: a preloaded library before the asan runtime
# aborts every sanitized binary at startup.
test-san:
	$(MAKE) clean
	LD_PRELOAD= $(MAKE) CFLAGS="-std=c99 -pedantic -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined" test
	$(MAKE) clean

# h1.1 gate: the shipped binary must import no libm transcendental and
# must not link libm at all.
gate-libm: helix
	@bad=$$(nm -u helix | grep -E '\b(expf|logf|sqrtf|sinf|cosf|sincosf|powf|tanhf|hypotf|fabsf)\b' || true); \
	 if [ -n "$$bad" ]; then echo "LIB_M_BOUNDARY: $$bad" >&2; exit 1; fi
	@if ldd helix 2>/dev/null | grep -q libm; then echo "LIB_M_BOUNDARY: libm linked" >&2; exit 1; fi
	@echo "gate-libm: ok, no libm boundary"

.PHONY: all test test-san clean gate-libm
