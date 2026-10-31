//! android harness (eframe 0.31): the AndroidApp goes into
//! NativeOptions.android_app; winit drives the activity loop, egui
//! renders — no ndk-glue, no custom event pump. on a desktop host the
//! same crate runs as a dev harness writing into helix-android-dev/.

#[cfg(not(target_os = "android"))]
fn main() {
    setup_desktop();
    let home = std::env::var("HELIX_HOME").unwrap_or_else(|_| ".".into());
    let _ = eframe::run_native(
        "helix",
        eframe::NativeOptions::default(),
        Box::new(move |cc| {
            Ok(Box::new(helix_gui::mobile::MobileApp::new(
                std::path::PathBuf::from(home),
                &cc.egui_ctx,
            )) as Box<dyn eframe::App>)
        }),
    );
}

#[cfg(target_os = "android")]
use android_activity::AndroidApp;

#[cfg(target_os = "android")]
#[no_mangle]
fn android_main(app: AndroidApp) {
    android_logger::init_once(
        android_logger::Config::default().with_max_level(log::LevelFilter::Info),
    );
    log::info!("helix android_main");

    let home = external_files_dir(&app)
        .unwrap_or_else(|| std::path::PathBuf::from("/data/local/tmp"));
    let _ = std::fs::create_dir_all(&home);
    /* the c core wants the connectome by path: the table ships inside
       the lib and is unpacked into the app files dir on first run */
    let csv = home.join("herm_302.csv");
    if !csv.exists() {
        let _ = std::fs::write(&csv, CONNECTOME);
    }
    std::env::set_var("HOME", &home);
    std::env::set_var("HELIX_HOME", &home);
    std::env::set_var("HELIX_CONNECTOME", &csv);
    /* the keychain lives in internal storage: external app dirs are
       reachable over adb/mtp, and the kch db has no reason to leave
       the sandbox */
    let kch_dir = internal_files_dir(&app).unwrap_or_else(|| home.clone());
    std::env::set_var("HELIX_KCH_DIR", &kch_dir);
    log::info!("helix home: {} kch: {}", home.display(), kch_dir.display());

    let options = eframe::NativeOptions {
        android_app: Some(app),
        ..Default::default()
    };
    let _ = eframe::run_native(
        "helix",
        options,
        Box::new(move |cc| {
            Ok(Box::new(helix_gui::mobile::MobileApp::new(
                home.clone(),
                &cc.egui_ctx,
            )) as Box<dyn eframe::App>)
        }),
    );
}

#[cfg(target_os = "android")]
const CONNECTOME: &str = include_str!("../../vendor/petri/data/herm_302.csv");

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

/* internal files dir via JNI: ctx.getFilesDir().getAbsolutePath() */
#[cfg(target_os = "android")]
fn internal_files_dir(app: &AndroidApp) -> Option<std::path::PathBuf> {
    let vm_ptr = app.vm_as_ptr();
    let act_ptr = app.activity_as_ptr();
    let vm = unsafe { jni::JavaVM::from_raw(vm_ptr.cast()) }.ok()?;
    let mut env = vm.attach_current_thread().ok()?;
    let activity = unsafe { jni::objects::JObject::from_raw(act_ptr.cast()) };
    let file = env
        .call_method(
            &activity,
            "getFilesDir",
            "()Ljava/io/File;",
            &[],
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

/* app-specific external files dir via JNI:
   ctx.getExternalFilesDir(null).getAbsolutePath() — adb and any mtp
   file manager can reach it, so files move on and off the device
   without a saf picker (roadmap item for v2) */
#[cfg(target_os = "android")]
fn external_files_dir(app: &AndroidApp) -> Option<std::path::PathBuf> {
    let vm_ptr = app.vm_as_ptr();
    let act_ptr = app.activity_as_ptr();
    let vm = unsafe { jni::JavaVM::from_raw(vm_ptr.cast()) }.ok()?;
    let mut env = vm.attach_current_thread().ok()?;
    let activity = unsafe { jni::objects::JObject::from_raw(act_ptr.cast()) };
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
