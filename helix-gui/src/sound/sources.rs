//! sample synthesis for the six constitution sources. every function
//! renders dry samples into the ring; the engine adds the room (convolver
//! wet 0.13), the compressor and the 14k lowpass.

/// fnv-1a over (mode, name, bucket): the constitution's deterministic
/// jitter source, so the same action sounds the same way twice
pub fn jitter_hash(mode: &str, name: &str, bucket: u64) -> u64 {
    let mut h: u64 = 0xcbf29ce484222325;
    for b in mode.bytes().chain(name.bytes()) {
        h ^= b as u64;
        h = h.wrapping_mul(0x100000001b3);
    }
    h ^= bucket;
    h = h.wrapping_mul(0x100000001b3);
    h
}

fn rng_next(state: &mut u64) -> u64 {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    state.wrapping_mul(2685821657736338717)
}

/// contact tick: 6 ms of high-passed noise, exponential decay
/* soft ui sound set (owner choice 2026-10-30): a delicate click, a
   gentle two-note chime and a low dud replace the contact/plate/hum/
   relay palette; everything sits far below full scale. */
pub fn soft_click(out: &mut Vec<f32>, sr: u32, seed: u64) {
    let n = (sr as usize) * 30 / 1000;
    let jitter = 0.9 + 0.2 * ((seed % 7) as f32) / 7.0;
    for i in 0..n {
        let t = i as f32 / sr as f32;
        let env = (-t * 220.0).exp();
        let v = (2.0 * std::f32::consts::PI * 1500.0 * jitter * t).sin()
            + 0.3 * (2.0 * std::f32::consts::PI * 3200.0 * t).sin();
        out.push(0.10 * env * v);
    }
}

pub fn chime(out: &mut Vec<f32>, sr: u32, seed: u64) {
    let n = (sr as usize) * 420 / 1000;
    let det = 1.0 + 0.01 * ((seed % 5) as f32) / 5.0;
    for i in 0..n {
        let t = i as f32 / sr as f32;
        let mut v = 0.0;
        for (k, f) in [(0.0f32, 1318.0f32), (0.07, 987.0f32)] {
            let ts = t - k as f32 * 0.07;
            if ts > 0.0 {
                v += (-ts * 9.0).exp()
                    * (2.0 * std::f32::consts::PI * f * det * ts).sin();
            }
        }
        out.push(0.07 * v);
    }
}

pub fn dud(out: &mut Vec<f32>, sr: u32) {
    let n = (sr as usize) * 180 / 1000;
    let mut phase = 0.0f32;
    for i in 0..n {
        let t = i as f32 / sr as f32;
        let f = 210.0 - 60.0 * (t / 0.18);
        phase += 2.0 * std::f32::consts::PI * f / sr as f32;
        out.push(0.09 * (-t * 14.0).exp() * phase.sin());
    }
}

pub fn tick(out: &mut Vec<f32>, sr: u32, seed: u64, gain: f32) {
    let n = (sr as usize * 6) / 1000;
    let mut st = seed | 1;
    let mut lp = 0.0f32;
    for i in 0..n {
        let w = ((rng_next(&mut st) >> 40) as f32 / 16777216.0 - 1.0) * gain;
        /* one-pole high pass at ~3 khz */
        let hp = w - lp;
        lp += 0.6 * w;
        let env = (-((i as f32) / (n as f32) * 9.0)).exp();
        out.push(hp * env);
    }
}

/// inharmonic plate: modes at 1 : 2.76 : 5.4 : 8.9 times the base, each
/// with its own decay, detuned by the seed
pub fn plate(out: &mut Vec<f32>, sr: u32, base: f32, seed: u64) {
    let dur_s = 1.1;
    let n = (sr as f32 * dur_s) as usize;
    let mut st = seed | 1;
    let cents = ((rng_next(&mut st) >> 58) as f32 - 32.0) / 32.0 * 0.12;
    let detune = 2.0f32.powf(cents / 12.0);
    let modes = [1.0f32, 2.76, 5.4, 8.9];
    let decays = [1.0f32, 0.55, 0.35, 0.22];
    let gains = [0.5f32, 0.28, 0.16, 0.09];
    out.resize(out.len() + n, 0.0);
    for (m, (&ratio, (&dec, &g))) in modes.iter().zip(decays.iter().zip(gains.iter())).enumerate() {
        let f = base * ratio * detune * (1.0 + 0.002 * m as f32);
        let w = 2.0 * std::f32::consts::PI * f / sr as f32;
        let tau = dec * 0.45;
        for i in 0..n {
            let t = i as f32 / sr as f32;
            out[i] += (w * i as f32).sin() * g * (-t / tau).exp();
        }
    }
}

/// body thump: a 70 hz sine burst, constitution delay 6 ms
pub fn thump(out: &mut Vec<f32>, sr: u32) {
    let lead = sr as usize * 6 / 1000;
    out.resize(out.len() + lead, 0.0);
    let n = sr as usize * 90 / 1000;
    for i in 0..n {
        let t = i as f32 / sr as f32;
        out.push((2.0 * std::f32::consts::PI * 70.0 * t).sin() * 0.5 * (-t / 0.04).exp());
    }
}

/// the line: bandpass noise sweeping 300 -> 2400 hz with packet bursts,
/// used while a transfer runs
pub fn line(out: &mut Vec<f32>, sr: u32, ms: u32, seed: u64) {
    let n = sr as usize * ms as usize / 1000;
    let mut st = seed | 1;
    let mut bp = 0.0f32;
    let mut bp2 = 0.0f32;
    for i in 0..n {
        let t = i as f32 / n as f32;
        let f = 300.0 + (2400.0 - 300.0) * t;
        let w = 2.0 * std::f32::consts::PI * f / sr as f32;
        let noise = (rng_next(&mut st) >> 40) as f32 / 16777216.0 - 1.0;
        /* two one-pole stages approximate the bandpass */
        bp += 0.15 * (noise - bp);
        bp2 += (w.min(1.5)) * (bp - bp2);
        /* packets: amplitude gates at 8 hz with jittered duty */
        let gate = if (t * 8.0).fract() < 0.55 { 1.0 } else { 0.05 };
        out.push(bp2 * 0.8 * gate);
    }
}

/// coil hum: 50 and 53.9 hz plus their 100 / 107.8 harmonics; the engine
/// keeps it running while the queue is alive
pub fn hum(out: &mut Vec<f32>, sr: u32, phase: &mut f32, on: bool) {
    let n = 512; /* one render block */
    for _ in 0..n {
        *phase += 2.0 * std::f32::consts::PI * 50.0 / sr as f32;
        let p1 = *phase;
        let p2 = p1 * 53.9 / 50.0;
        let v = 0.30 * p1.sin()
            + 0.22 * p2.sin()
            + 0.10 * (2.0 * p1).sin()
            + 0.07 * (2.0 * p2).sin();
        out.push(if on { v * 0.06 } else { 0.0 });
    }
}

/// fault: relay amplitude modulation at 100 hz over a low saw, plus the
/// dull double contact
pub fn fault(out: &mut Vec<f32>, sr: u32, seed: u64) {
    let n = sr as usize * 320 / 1000;
    let mut st = seed | 1;
    let _ = &mut st;
    let mut saw_phase = 0.0f32;
    for i in 0..n {
        let t = i as f32 / sr as f32;
        saw_phase += 120.0 / sr as f32;
        if saw_phase > 1.0 {
            saw_phase -= 1.0;
        }
        let am = 0.5 + 0.5 * (2.0 * std::f32::consts::PI * 100.0 * t).sin();
        let saw = 2.0 * saw_phase - 1.0;
        out.push(saw * 0.18 * am * (-t / 0.28).exp());
    }
    /* secondary dull contact at +42 ms */
    let lead = sr as usize * 42 / 1000;
    let mut second = Vec::new();
    tick(&mut second, sr, seed ^ 0x5a5a, 0.25);
    for (i, v) in second.into_iter().enumerate() {
        if lead + i < out.len() {
            out[lead + i] += v;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /* the render functions must produce bounded output and the hash must
       be stable for the same inputs (deterministic jitter contract) */
    #[test]
    fn sources_render_bounded_and_hash_stable() {
        let sr = 48000u32;
        let mut buf = Vec::new();
        plate(&mut buf, sr, 220.0, 42);
        assert!(!buf.is_empty());
        assert!(buf.iter().all(|v| v.is_finite() && v.abs() <= 1.5));
        assert_eq!(jitter_hash("tick", "a.dna", 7), jitter_hash("tick", "a.dna", 7));
        assert_ne!(jitter_hash("tick", "a.dna", 7), jitter_hash("tick", "b.dna", 7));
        let mut hum_buf = Vec::new();
        let mut ph = 0.0;
        hum(&mut hum_buf, sr, &mut ph, true);
        assert!(hum_buf.iter().all(|v| v.abs() <= 0.2));
    }
}
