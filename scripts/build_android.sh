#!/bin/sh
# android universal (fat) apk build. prereqs:
#   sudo apt install android-sdk-platform-tools-common  (or full studio sdk)
#   ANDROID_HOME with an ndk (r25c+) installed via sdkmanager
#   rustup target add aarch64-linux-android armv7-linux-androideabi i686-linux-android x86_64-linux-android
#   cargo install cargo-apk
set -e
: "${ANDROID_HOME:?export ANDROID_HOME=/path/to/android-sdk first}"
for t in aarch64-linux-android armv7-linux-androideabi i686-linux-android x86_64-linux-android; do
    rustup target add "$t" 2>/dev/null || true
done
command -v cargo-apk >/dev/null 2>&1 || cargo install cargo-apk

NDK_DIR="$(ls -d "$ANDROID_HOME"/ndk/* 2>/dev/null | sort -V | tail -1)"
[ -n "$NDK_DIR" ] || { echo "no ndk under $ANDROID_HOME/ndk"; exit 1; }
TOOLCHAIN="$NDK_DIR/toolchains/llvm/prebuilt/linux-x86_64/bin"

export CC_aarch64_linux_android="$TOOLCHAIN/aarch64-linux-android26-clang"
export AR_aarch64_linux_android="$TOOLCHAIN/llvm-ar"
export RANLIB_aarch64_linux_android="$TOOLCHAIN/llvm-ranlib"
export CC_armv7_linux_androideabi="$TOOLCHAIN/armv7a-linux-androideabi26-clang"
export AR_armv7_linux_androideabi="$TOOLCHAIN/llvm-ar"
export RANLIB_armv7_linux_androideabi="$TOOLCHAIN/llvm-ranlib"
export CC_i686_linux_android="$TOOLCHAIN/i686-linux-android26-clang"
export AR_i686_linux_android="$TOOLCHAIN/llvm-ar"
export RANLIB_i686_linux_android="$TOOLCHAIN/llvm-ranlib"
export CC_x86_64_linux_android="$TOOLCHAIN/x86_64-linux-android26-clang"
export AR_x86_64_linux_android="$TOOLCHAIN/llvm-ar"
export RANLIB_x86_64_linux_android="$TOOLCHAIN/llvm-ranlib"

cd "$(dirname "$0")/../helix-android"
cargo apk build --release

# reproducibility manifest (determinism gate): binary shas per abi +
# toolchain versions + compile flags
mkdir -p ../results/release
M=../results/release/android_manifest.txt
{
  echo "# helix android manifest: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "ndk: $(basename "$NDK_DIR")"
  rustc -vV | sed 's/^/rustc /'
  echo "targets: aarch64-linux-android armv7-linux-androideabi i686-linux-android x86_64-linux-android"
  for so in target/aarch64-linux-android/release/libhelix_android.so \
            target/armv7-linux-androideabi/release/libhelix_android.so \
            target/i686-linux-android/release/libhelix_android.so \
            target/x86_64-linux-android/release/libhelix_android.so; do
    [ -f "$so" ] && echo "$(sha256sum "$so")"
  done
} > "$M"
cat "$M"
echo
echo "Universal APK: $(cd ../target/release/apk && pwd)/helix.apk"
echo "install: adb install -r target/release/apk/helix.apk"
echo
echo "NOTE: the built APK is UNSIGNED (debug key). For distribution:"
echo "  1. Generate a release keystore once:"
echo "     keytool -genkey -v -keystore helix-release.keystore -alias helix -keyalg RSA -keysize 2048 -validity 10000"
echo "  2. Sign with apksigner (from build-tools):"
echo "     apksigner sign --ks helix-release.keystore --ks-key-alias helix target/release/apk/helix.apk"
echo "  3. Verify: apksigner verify target/release/apk/helix.apk"
