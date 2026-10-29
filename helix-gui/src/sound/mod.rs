//! sound: procedural physical engine per constitution §8. off by
//! default; every event has a visual twin drawn by the caller. the cpal
//! backend sits behind the `audio` feature (needs libasound2-dev on
//! debian); without it the handle is silent and the ui says so.

pub mod engine;
pub mod sources;

use std::sync::mpsc;

#[derive(Clone)]
pub enum Event {
    /// short contact tick on a button press; bucket varies the pitch
    Tick { bucket: u64 },
    /// completion plate strike (the nacre sweep is its visual twin)
    Plate { bucket: u64 },
    /// dull double contact on a fault; the ui glitch is its twin
    Fault { bucket: u64 },
    /// transfer line: bandpass sweep with packet bursts
    Line { ms: u32, bucket: u64 },
    /// coil hum while the job queue is alive
    Hum { on: bool },
}

enum Msg {
    Play(Event),
    Shutdown,
}

pub struct SoundHandle {
    tx: Option<mpsc::Sender<Msg>>,
    pub available: bool,
}

impl Default for SoundHandle {
    fn default() -> Self {
        SoundHandle { tx: None, available: false }
    }
}

impl SoundHandle {
    /// starts the engine thread; returns a silent handle when the audio
    /// feature is off or the device cannot be opened
    pub fn connect() -> SoundHandle {
        #[cfg(feature = "audio")]
        {
            match engine::spawn() {
                Some(tx) => SoundHandle { tx: Some(tx), available: true },
                None => SoundHandle::default(),
            }
        }
        #[cfg(not(feature = "audio"))]
        SoundHandle::default()
    }

    pub fn play(&self, ev: Event) {
        if let Some(tx) = &self.tx {
            let _ = tx.send(Msg::Play(ev));
        }
    }

    pub fn shutdown(&self) {
        if let Some(tx) = &self.tx {
            let _ = tx.send(Msg::Shutdown);
        }
    }
}
