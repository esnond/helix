# helix: biologically inspired key manager.
# c99, posix subset, no external libraries. libm for the vendored engine.

CC       = cc
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Werror -O2 -ffp-contract=off
CPPFLAGS = -D_DEFAULT_SOURCE -Ivendor/petri -Isrc
LDLIBS   = -lm

VENDOR_SRC = sim connectome neuron body world modulators plasticity \
             metrics rng gcamp mutants evolution
VENDOR_OBJ = $(addprefix vendor/petri/,$(addsuffix .o,$(VENDOR_SRC)))
CRYPTO_SRC = hex sha256 sha512 hmac pbkdf2 bip39
CRYPTO_OBJ = $(addprefix src/,$(addsuffix .o,$(CRYPTO_SRC)))
TESTS      = tests/test_vectors tests/test_bip39

all: libpetri.a $(TESTS)

vendor/petri/%.o: vendor/petri/%.c $(wildcard vendor/petri/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

libpetri.a: $(VENDOR_OBJ)
	ar rcs $@ $(VENDOR_OBJ)

src/%.o: src/%.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tests/test_%: tests/test_%.o $(CRYPTO_OBJ) libpetri.a
	$(CC) $(CFLAGS) -o $@ $< $(CRYPTO_OBJ) libpetri.a $(LDLIBS)

tests/test_%.o: tests/test_%.c $(wildcard src/*.h) $(wildcard tests/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itests -c -o $@ $<

test: libpetri.a $(TESTS)
	sh tests/run_tests.sh

clean:
	rm -f vendor/petri/*.o src/*.o libpetri.a tests/test_* tests/test_*.o helix

.PHONY: all test clean
