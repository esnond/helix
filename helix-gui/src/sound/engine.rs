//! the audio engine thread: owns the cpal stream (behind the `audio`
//! feature), renders events into a ring, and shapes the output with the
//! constitution signal chain: convolver (procedural metal-box ir, wet
//! 0.13) -> compressor -> lowpass 14k -> out.
//! without the feature this file is a silent stub.

#[cfg(feature = "audio")]
mod active {
    use super::super::{Event, Msg};
    use crate::sound::sources;
    use std::sync::mpsc;
    use std::sync::{Arc, Mutex};

    /// procedural metal-box ir: early reflections plus an exponentially
    /// decaying noise tail, ~10 ms
    fn metal_box_ir(sr: u32, taps: usize) -> Vec<f32> {
        let mut st: u64 = 0x6d6574616c626f78;
        let mut ir = vec![0.0f32; taps];
        for (pos, g) in [(0usize, 0.9f32), (97, 0.6), (211, 0.45), (359, 0.3)] {
            if pos < taps {
                ir[pos] += g;
            }
        }
        for i in 0..taps {
            let t = i as f32 / sr as f32;
            let noise = ((st >> 40) as f32 / 16777216.0 - 1.0) * 0.12;
            st ^= st >> 12;
            st ^= st << 25;
            st ^= st >> 27;
            ir[i] += noise * (-t / 0.004).exp();
        }
        ir
    }

    struct Chain {
        ir: Vec<f32>,
        lp_state: f32,
        lp_a: f32,
    }

    impl Chain {
        fn new(sr: u32) -> Chain {
            let taps = (sr as usize * 10 / 1000).max(64);
            Chain {
                ir: metal_box_ir(sr, taps),
                lp_state: 0.0,
                lp_a: 1.0 - (-2.0 * std::f32::consts::PI * 14000.0 / sr as f32).exp(),
            }
        }

        /// dry + 0.13 * conv(ir), soft compressor above -12 dbfs, then
        /// the one-pole 14 khz lowpass
        fn process(&mut self, block: &mut [f32]) {
            let ir = self.ir.clone();
            let wet_gain = 0.13f32;
            for i in 0..block.len() {
                let mut wet = 0.0f32;
                for (k, &c) in ir.iter().enumerate() {
                    if k > i {
                        break;
                    }
                    wet += c * block[i - k];
                }
                let mut v = block[i] + wet_gain * wet;
                let a = v.abs();
                if a > 0.25 {
                    v *= 0.25 + (a - 0.25) * 0.4 / a;
                }
                self.lp_state += self.lp_a * (v - self.lp_state);
                block[i] = self.lp_state;
            }
        }
    }

    fn render_event(
        ring: &Arc<Mutex<Vec<f32>>>, sr: u32, ev: &Event, hum_on: &mut bool,
    ) {
        let mut buf = Vec::new();
        match ev {
            Event::Tick { bucket } => {
                let seed = sources::jitter_hash("tick", "press", *bucket);
                sources::tick(&mut buf, sr, seed, 0.5);
                sources::thump(&mut buf, sr);
            }
            Event::Plate { bucket } => {
                let seed = sources::jitter_hash("plate", "done", *bucket);
                sources::plate(&mut buf, sr, 220.0, seed);
                sources::tick(&mut buf, sr, seed ^ 1, 0.4);
            }
            Event::Fault { bucket } => {
                let seed = sources::jitter_hash("fault", "error", *bucket);
                sources::fault(&mut buf, sr, seed);
            }
            Event::Line { ms, bucket } => {
                let seed = sources::jitter_hash("line", "transfer", *bucket);
                sources::line(&mut buf, sr, *ms, seed);
            }
            Event::Hum { on } => {
                *hum_on = *on;
                return;
            }
        }
        if let Ok(mut r) = ring.lock() {
            r.extend_from_slice(&buf);
        }
    }

    pub fn spawn() -> Option<mpsc::Sender<Msg>> {
        use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
        let host = cpal::default_host();
        let device = host.default_output_device()?;
        let cfg = device.default_output_config().ok()?;
        let sr = cfg.sample_rate().0;
        let channels = cfg.channels() as usize;
        let (tx, rx) = mpsc::channel::<Msg>();
        let ring: Arc<Mutex<Vec<f32>>> = Arc::new(Mutex::new(Vec::new()));
        let ring2 = ring.clone();
        let mut chain = Chain::new(sr);
        let mut hum_phase = 0.0f32;
        let mut hum_on = false;

        let stream = device
            .build_output_stream(
                &cfg.into(),
                move |out: &mut [f32], _: &cpal::OutputCallbackInfo| {
                    loop {
                        match rx.try_recv() {
                            Ok(Msg::Shutdown) => break,
                            Ok(Msg::Play(ev)) => {
                                render_event(&ring2, sr, &ev, &mut hum_on)
                            }
                            Err(_) => break,
                        }
                    }
                    let want = out.len() / channels;
                    let mut block = {
                        let mut r = ring2.lock().unwrap();
                        let take = r.len().min(want);
                        r.drain(..take).collect::<Vec<f32>>()
                    };
                    if hum_on && block.len() < want {
                        let mut hb = Vec::new();
                        sources::hum(&mut hb, sr, &mut hum_phase, true);
                        block.truncate(want);
                        block.extend_from_slice(&hb);
                    }
                    block.resize(want, 0.0);
                    chain.process(&mut block);
                    for (i, v) in block.into_iter().enumerate() {
                        for c in 0..channels {
                            out[i * channels + c] = v;
                        }
                    }
                },
                move |err| {
                    let _ = err;
                },
            )
            .ok()?;
        stream.play().ok()?;
        std::thread::spawn(move || {
            let _stream = stream;
            loop {
                std::thread::sleep(std::time::Duration::from_millis(500));
            }
        });
        Some(tx)
    }
}

#[cfg(feature = "audio")]
pub use active::spawn;

#[cfg(not(feature = "audio"))]
pub fn spawn() -> Option<std::sync::mpsc::Sender<super::Msg>> {
    None
}
