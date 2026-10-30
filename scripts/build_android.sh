#!/bin/sh
# android apk build. prereqs:
#   sudo apt install android-sdk-platform-tools-common  (or full studio sdk)
#   ANDROID_HOME with an ndk (r25c+) installed via sdkmanager
#   rustup target add aarch64-linux-android
#   cargo install cargo-apk
set -e
: "${ANDROID_HOME:?export ANDROID_HOME=/path/to/android-sdk first}"
rustup target add aarch64-linux-android
command -v cargo-apk >/dev/null 2>&1 || cargo install cargo-apk

NDK_DIR="$(ls -d "$ANDROID_HOME"/ndk/* 2>/dev/null | sort -V | tail -1)"
[ -n "$NDK_DIR" ] || { echo "no ndk under $ANDROID_HOME/ndk"; exit 1; }
TOOLCHAIN="$NDK_DIR/toolchains/llvm/prebuilt/linux-x86_64/bin"
export CC_aarch64_linux_android="$TOOLCHAIN/aarch64-linux-android26-clang"
export AR_aarch64_linux_android="$TOOLCHAIN/llvm-ar"
export RANLIB_aarch64_linux_android="$TOOLCHAIN/llvm-ranlib"

cd "$(dirname "$0")/../helix-android"
cargo apk build --release
echo
echo "APK: $(cd ../target/release/apk && pwd)/helix.apk"
echo "install: adb install -r target/release/apk/helix.apk"
