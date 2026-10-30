//! helix-gui library: the desktop binary and the android app share
//! everything here. one unsafe boundary (core/helix_wrapper.rs over
//! gui_api.c); no crypto in rust.

#[path = "core/helix_wrapper.rs"]
pub mod wrapper;

pub mod app;
pub mod bar_status;
pub mod dialog;
pub mod fonts;
pub mod i18n;
pub mod jobs;
pub mod mobile;
pub mod panel_files;
pub mod panel_jobs;
pub mod panel_keys;
pub mod pixel;
pub mod prefs;
pub mod sound;
pub mod theme;
