/* build.rs: compiles the helix c core (vendored engine + crypto +
   gui_api shim) into the rust binary with the same flags as the c
   gate. keep in sync with the root Makefile. */
fn main() {
    println!("cargo:rustc-link-lib=m");
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=../src");
    println!("cargo:rerun-if-changed=../vendor/petri");

    let mut build = cc::Build::new();
    build
        .std("c99")
        .flag("-pedantic")
        .flag("-Wall")
        .flag("-Wextra")
        .flag("-Werror")
        .flag("-ffp-contract=off")
        .flag("-D_DEFAULT_SOURCE")
        .include("../vendor/petri")
        .include("../src")
        .include("src");

    let vendor = [
        "sim", "connectome", "neuron", "body", "world", "modulators",
        "plasticity", "metrics", "rng", "gcamp", "mutants", "evolution",
    ];
    for u in vendor {
        build.file(format!("../vendor/petri/{u}.c"));
    }
    let helix_src = [
        "hex", "sha256", "sha512", "hmac", "pbkdf2", "bip39", "entropy",
        "kdf", "keychain", "evolve", "keycheck", "chacha20", "poly1305",
        "aead", "dna",
    ];
    for u in helix_src {
        build.file(format!("../src/{u}.c"));
    }
    build.file("src/gui_api.c");
    build.compile("helix_core");
}
