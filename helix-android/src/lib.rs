//! android harness: prepares HOME and the connectome in the app's
//! external files dir, then runs the same 1-bit mac ui as the desktop.
//! on a desktop host it runs unchanged for development (cargo check /
//! cargo run without the ndk), writing into helix-android-dev/.

use android_activity::{AndroidApp, MainEvent, PollEvent};
use log::LevelFilter;

#[cfg(not(target_os = "android"))]
fn main() {
    setup_desktop();
    let home = std::env::var("HELIX_HOME").unwrap_or_else(|_| ".".into());
    let _ = eframe::run_native(
        "helix",
        eframe::NativeOptions::default(),
        Box::new(move |cc| {
            Box::new(helix_gui::mobile::MobileApp::new(
                std::path::PathBuf::from(home),
                &cc.egui_ctx,
            )) as Box<dyn eframe::App>
        }),
    );
}

#[cfg(target_os = "android")]
#[no_mangle]
fn android_main(app: AndroidApp) {
    android_logger::init_once(
        android_logger::Config::default().with_max_level(LevelFilter::Info),
    );
    log::info!("helix android_main started");

    let home = external_files_dir(&app).unwrap_or_else(|| std::path::PathBuf::from("/data/local/tmp"));
    let _ = std::fs::create_dir_all(&home);
    let csv = home.join("herm_302.csv");
    if !csv.exists() {
        let _ = std::fs::write(&csv, CONNECTOME);
    }
    std::env::set_var("HOME", &home);
    std::env::set_var("HELIX_HOME", &home);
    std::env::set_var("HELIX_CONNECTOME", &csv);
    log::info!("helix home: {}", home.display());

    let mut app = app;
    // TODO: integrate egui rendering loop here
    loop {
        app.poll_events(Some(std::time::Duration::from_millis(16)), |event| {
            match event {
                PollEvent::Main(MainEvent::Destroy) => {
                    log::info!("Destroy event");
                    return;
                }
                PollEvent::Main(MainEvent::LowMemory) => {
                    log::warn!("Low memory");
                }
                PollEvent::Main(MainEvent::SaveState { .. }) => {
                    log::info!("Save state");
                }
                PollEvent::Main(MainEvent::Start) => {
                    log::info!("Start");
                }
                PollEvent::Main(MainEvent::Resume { .. }) => {
                    log::info!("Resume");
                }
                PollEvent::Main(MainEvent::Pause) => {
                    log::info!("Pause");
                }
                PollEvent::Main(MainEvent::Stop) => {
                    log::info!("Stop");
                }
                PollEvent::Main(MainEvent::ConfigChanged { .. }) => {
                    log::info!("Config changed");
                }
                _ => {}
            }
        });
    }
}

#[cfg(not(target_os = "android"))]
fn setup_desktop() {
    let dir = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../helix-android-dev");
    let _ = std::fs::create_dir_all(&dir);
    std::env::set_var("HOME", &dir);
    std::env::set_var("HELIX_HOME", &dir);
    let csv = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("../vendor/petri/data/herm_302.csv");
    std::env::set_var("HELIX_CONNECTOME", &csv);
}

#[cfg(target_os = "android")]
const CONNECTOME: &str = include_str!("../../vendor/petri/data/herm_302.csv");

#[cfg(target_os = "android")]
fn external_files_dir(_app: &AndroidApp) -> Option<std::path::PathBuf> {
    let ctx = ndk_context::android_context();
    let vm = unsafe { jni::JavaVM::from_raw(ctx.vm().cast()) }.ok()?;
    let mut env = vm.attach_current_thread().ok()?;
    let activity = unsafe { jni::objects::JObject::from_raw(ctx.context().cast()) };
    let file = env
        .call_method(
            &activity,
            "getExternalFilesDir",
            "(Ljava/lang/String;)Ljava/io/File;",
            &[jni::objects::JValue::Object(&jni::objects::JObject::null())],
        )
        .ok()?
        .l()
        .ok()?;
    let path = env
        .call_method(&file, "getAbsolutePath", "()Ljava/lang/String;", &[])
        .ok()?
        .l()
        .ok()?;
    let s = env
        .get_string(&jni::objects::JString::from(path))
        .ok()?
        .to_string_lossy()
        .into_owned();
    Some(std::path::PathBuf::from(s))
}