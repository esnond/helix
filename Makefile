# helix: biologically inspired key manager.
# c99, posix subset, no external libraries. libm for the vendored engine.

CC       = cc
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Werror -O2 -ffp-contract=off
CPPFLAGS = -D_DEFAULT_SOURCE -Ivendor/petri -Isrc
LDLIBS   = -lm

VENDOR_SRC = sim connectome neuron body world modulators plasticity \
             metrics rng gcamp mutants evolution
VENDOR_OBJ = $(addprefix vendor/petri/,$(addsuffix .o,$(VENDOR_SRC)))

all: libpetri.a

vendor/petri/%.o: vendor/petri/%.c $(wildcard vendor/petri/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

libpetri.a: $(VENDOR_OBJ)
	ar rcs $@ $(VENDOR_OBJ)

test: libpetri.a
	sh tests/run_tests.sh

clean:
	rm -f vendor/petri/*.o libpetri.a tests/test_* helix

.PHONY: all test clean
