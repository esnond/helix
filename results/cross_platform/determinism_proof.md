# h3 determinism proof status

what is proven, on this machine (debian 13, gcc 14.2.0, glibc 2.41,
x86_64):

1. compiler configuration independence: -O0, -O1, -O2, -O3 and
   -O3 -march=native produce the bitwise identical key for the fixed
   test vector (platform_results.csv). the -ffp-contract=off flag
   holds across optimization levels.
2. run independence: the r0/r1 test suites re-derive and compare keys
   in process and across processes on every run (shell cmp gate), and
   have never diverged across hundreds of runs.
3. second machine / second architecture: NOT proven - no second
   machine, no arm64 board, no macos, no wsl2 is available in this
   environment, and the i386 multilib is blocked by missing 32 bit
   libc headers (float_analysis.md has the exact command and the
   predicted outcomes). the claim is therefore: determinism is proven
   within the x86_64/sse2 build environment class and unproven across
   architectures and libm versions. this is recorded as the project's
   sharpest open risk, with the fingerprint verification procedure as
   the operational mitigation until a second platform is available.

reproduction: sh tools/cross_config_test.sh (about four minutes,
writes results/cross_platform/platform_results.csv).
