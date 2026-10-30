//! desktop entry: android lives in helix-android and reuses the
//! helix_gui library.

use eframe::egui;

use helix_gui::{app, fonts, prefs};

fn resolve_connectome() {
    if std::env::var_os("HELIX_CONNECTOME").is_some() {
        return;
    }
    /* the c core opens the table relative to the cwd. launched from the
       desktop the cwd is $HOME, so try the build tree and the
       compile-time location last */
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.to_path_buf()));
    let mut candidates: Vec<String> = vec![
        concat!(env!("CARGO_MANIFEST_DIR"), "/../vendor/petri/data/herm_302.csv")
            .to_string(),
    ];
    if let Some(d) = exe_dir {
        /* installed layout: ~/.local/bin binary, table next to it */
        candidates.push(
            d.join("vendor/petri/data/herm_302.csv").to_string_lossy().into_owned(),
        );
    }
    for c in &candidates {
        if std::path::Path::new(c).exists() {
            std::env::set_var("HELIX_CONNECTOME", c);
            return;
        }
    }
    for c in candidates {
        if std::path::Path::new(c.as_str()).exists() {
            std::env::set_var("HELIX_CONNECTOME", c);
            return;
        }
    }
}

fn wants_fullscreen() -> bool {
    if std::env::args().any(|a| a == "--fullscreen" || a == "-f") {
        return true;
    }
    matches!(
        std::env::var("HELIX_FULLSCREEN").as_deref(),
        Ok("1") | Ok("true") | Ok("yes")
    )
}

fn main() -> eframe::Result<()> {
    resolve_connectome();
    let prefs = prefs::Prefs::load();
    prefs::Prefs::apply(&prefs);
    let opts = eframe::NativeOptions {
        initial_window_size: Some(egui::vec2(prefs.win.0, prefs.win.1)),
        fullscreen: wants_fullscreen(),
        ..Default::default()
    };
    eframe::run_native(
        "helix",
        opts,
        Box::new(move |cc| {
            fonts::apply(&cc.egui_ctx);
            Box::new(app::App::new(prefs)) as Box<dyn eframe::App>
        }),
    )
}
