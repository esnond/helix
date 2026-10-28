//! file manager: the central panel. dwm-like: borderless list, one
//! highlighted row, keyboard first. rows are painted manually so the
//! selection highlight and the text share one rect.

use eframe::egui::{self, pos2, Align2, FontId, Sense};
use std::time::UNIX_EPOCH;

use crate::style::{colors, effects};

pub struct FileEntry {
    pub name: String,
    pub path: String,
    pub size: u64,
    pub is_dna: bool,
    pub generation: Option<u32>,
    pub mtime: Option<u64>,
}

/// scans one directory; parses the .dna header fields the gui shows
pub fn scan(dir: &std::path::Path) -> Vec<FileEntry> {
    let mut out = Vec::new();
    let Ok(rd) = std::fs::read_dir(dir) else {
        return out;
    };
    for e in rd.flatten() {
        let Ok(meta) = e.metadata() else { continue };
        if !meta.is_file() {
            continue;
        }
        let name = e.file_name().to_string_lossy().into_owned();
        let path = e.path().to_string_lossy().into_owned();
        let is_dna = name.ends_with(".dna");
        let generation = if is_dna { parse_generation(&path) } else { None };
        let mtime = std::fs::metadata(&path)
            .and_then(|m| m.modified())
            .ok()
            .and_then(|t| t.duration_since(UNIX_EPOCH).ok())
            .map(|d| d.as_secs());
        out.push(FileEntry {
            name,
            path,
            size: meta.len(),
            is_dna,
            generation,
            mtime,
        });
    }
    out.sort_by(|a, b| a.name.cmp(&b.name));
    out
}

/// header offset 41..45 is the generation, big endian
fn parse_generation(path: &str) -> Option<u32> {
    let data = std::fs::read(path).ok()?;
    if data.len() < 45 || &data[0..4] != b"DNA1" {
        return None;
    }
    Some(u32::from_be_bytes([data[41], data[42], data[43], data[44]]))
}

fn human(n: u64) -> String {
    if n >= 1 << 20 {
        format!("{:.1}M", n as f64 / (1 << 20) as f64)
    } else if n >= 1 << 10 {
        format!("{:.1}K", n as f64 / (1 << 10) as f64)
    } else {
        format!("{n}B")
    }
}

const COL_NAME: f32 = 8.0;
const COL_SIZE: f32 = 330.0;
const COL_GEN: f32 = 400.0;
const COL_MTIME: f32 = 450.0;

fn row_text(
    painter: &egui::Painter, rect: egui::Rect, f: &FileEntry,
    sel: bool, gen_known: bool,
) {
    let color = if sel {
        colors::TEXT
    } else if f.is_dna {
        colors::SECONDARY
    } else {
        colors::TEXT_DIM
    };
    let font = FontId::monospace(12.0);
    painter.text(
        pos2(rect.left() + COL_NAME, rect.center().y),
        Align2::LEFT_CENTER,
        &f.name,
        font.clone(),
        color,
    );
    painter.text(
        pos2(rect.left() + COL_SIZE, rect.center().y),
        Align2::LEFT_CENTER,
        human(f.size),
        font.clone(),
        color,
    );
    let gen = if !f.is_dna {
        "-".to_string()
    } else if gen_known {
        f.generation.map(|g| g.to_string()).unwrap_or_else(|| "?".into())
    } else {
        "-".to_string()
    };
    painter.text(
        pos2(rect.left() + COL_GEN, rect.center().y),
        Align2::LEFT_CENTER,
        gen,
        font.clone(),
        color,
    );
    painter.text(
        pos2(rect.left() + COL_MTIME, rect.center().y),
        Align2::LEFT_CENTER,
        f.mtime.map(|t| t.to_string()).unwrap_or_default(),
        font,
        color,
    );
}

/// draws the list. returns the clicked row (mouse), the caller keeps
/// keyboard state.
pub fn show(
    ui: &mut egui::Ui, files: &[FileEntry], selected: &mut usize,
    glow: f32,
) {
    let font = FontId::monospace(12.0);
    egui::ScrollArea::vertical().show(ui, |ui| {
        if files.is_empty() {
            ui.add_space(20.0);
            ui.colored_label(colors::TEXT_DIM, "no files in this directory");
            return;
        }
        if *selected >= files.len() {
            *selected = files.len() - 1;
        }
        // header
        let (hrect, _) = ui.allocate_exact_size(
            egui::vec2(ui.available_width(), 18.0),
            Sense::hover(),
        );
        let hp = ui.painter_at(hrect);
        for (x, t) in [
            (COL_NAME, "name"),
            (COL_SIZE, "size"),
            (COL_GEN, "gen"),
            (COL_MTIME, "mtime"),
        ] {
            hp.text(
                pos2(hrect.left() + x, hrect.center().y),
                Align2::LEFT_CENTER,
                t,
                font.clone(),
                colors::ACCENT,
            );
        }
        for (i, f) in files.iter().enumerate() {
            let sel = i == *selected;
            let (rect, resp) = ui.allocate_exact_size(
                egui::vec2(ui.available_width(), 20.0),
                Sense::click(),
            );
            if resp.clicked() {
                *selected = i;
            }
            let painter = ui.painter_at(rect);
            if sel {
                painter.rect_filled(
                    rect,
                    2.0,
                    colors::PRIMARY.gamma_multiply(0.30),
                );
                effects::glow(&painter, rect, colors::ACCENT, glow);
            }
            row_text(&painter, rect, f, sel, true);
        }
    });
}
