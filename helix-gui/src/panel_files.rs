//! the file panel: path row with up/refresh, the action toolbar, and the
//! list. every action is a button; the list rows are clickable. empty
//! and read-failure states carry a cause and one next action.

use eframe::egui::{self, pos2, Align2, Sense};
use std::time::UNIX_EPOCH;

use crate::i18n;
use crate::theme;

pub struct FileEntry {
    pub name: String,
    pub path: String,
    pub size: u64,
    pub is_dir: bool,
    pub is_dna: bool,
    pub generation: Option<u32>,
    pub mtime: Option<u64>,
}

pub enum ListState {
    Ok,
    Empty,
    ReadFailed(String),
}

pub enum Action {
    None,
    Up,
    Refresh,
    /// double click or enter on a row: folder or file
    Activate(usize),
    Encrypt(String),
    Decrypt(String),
    Delete(String),
}

/// scans one directory; directories are navigable rows, sorted first
pub fn scan(dir: &std::path::Path) -> Result<Vec<FileEntry>, String> {
    let rd = std::fs::read_dir(dir).map_err(|e| e.to_string())?;
    let mut out = Vec::new();
    for e in rd.flatten() {
        let Ok(meta) = e.metadata() else { continue };
        let is_dir = meta.is_dir();
        let name = e.file_name().to_string_lossy().into_owned();
        let path = e.path().to_string_lossy().into_owned();
        let is_dna = !is_dir && name.ends_with(".dna");
        let generation = if is_dna { parse_generation(&path) } else { None };
        let mtime = std::fs::metadata(&path)
            .and_then(|m| m.modified())
            .ok()
            .and_then(|t| t.duration_since(UNIX_EPOCH).ok())
            .map(|d| d.as_secs());
        out.push(FileEntry { name, path, size: meta.len(), is_dir, is_dna, generation, mtime });
    }
    out.sort_by(|a, b| match (a.is_dir, b.is_dir) {
        (true, false) => std::cmp::Ordering::Less,
        (false, true) => std::cmp::Ordering::Greater,
        _ => a.name.cmp(&b.name),
    });
    Ok(out)
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

pub fn show(
    ui: &mut egui::Ui, cwd: &str, files: &[FileEntry], list: &ListState,
    selected: &mut usize, now: f64, nacre: Option<f64>,
) -> Action {
    let t = theme::theme();
    let s = i18n::s();
    let mut action = Action::None;

    /* path row: the affordance is on screen, not in a man page */
    ui.horizontal(|ui| {
        ui.label(
            egui::RichText::new(format!("> {}", s.files))
                .font(crate::fonts::font_id(crate::fonts::SZ_BODY, false))
                .color(t.accent),
        );
        ui.add_space(8.0);
        if small_tool(ui, "..", s.tip_up).clicked() {
            action = Action::Up;
        }
        if small_tool(ui, "⟳", s.tip_refresh).clicked() {
            action = Action::Refresh;
        }
        ui.label(
            egui::RichText::new(cwd)
                .font(crate::fonts::font_id(12.0, true))
                .color(t.ink2),
        );
        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
            let sel = files.get(*selected);
            let is_file = sel.map(|f| !f.is_dir).unwrap_or(false);
            let is_dna = sel.map(|f| f.is_dna).unwrap_or(false);
            let sel_path = sel.map(|f| f.path.clone());
            if ui
                .add_enabled(
                    is_dna,
                    egui::Button::new(s.decrypt).min_size(egui::vec2(104.0, 26.0)),
                )
                .on_hover_text(s.tip_decrypt)
                .on_hover_cursor(egui::CursorIcon::PointingHand)
                .clicked()
            {
                action = Action::Decrypt(sel_path.clone().unwrap_or_default());
            }
            if ui
                .add_enabled(
                    is_file && !is_dna,
                    egui::Button::new(s.encrypt)
                        .fill(t.accent2)
                        .min_size(egui::vec2(104.0, 26.0)),
                )
                .on_hover_text(s.tip_encrypt)
                .on_hover_cursor(egui::CursorIcon::PointingHand)
                .clicked()
            {
                action = Action::Encrypt(sel_path.clone().unwrap_or_default());
            }
            if ui
                .add_enabled(
                    is_file && !is_dna,
                    egui::Button::new(s.delete).min_size(egui::vec2(84.0, 26.0)),
                )
                .on_hover_text(s.tip_delete)
                .on_hover_cursor(egui::CursorIcon::PointingHand)
                .clicked()
            {
                action = Action::Delete(sel_path.unwrap_or_default());
            }
        });
    });
    ui.add_space(2.0);
    ui.separator();

    /* nacre: a 1 px brass sweep across the top on completion */
    if let Some(t0) = nacre {
        let p = ((now - t0) / 0.6).clamp(0.0, 1.0);
        if p < 1.0 {
            let (rect, _) = ui.allocate_exact_size(
                egui::vec2(ui.available_width(), 1.4),
                Sense::hover(),
            );
            let x = rect.left() + rect.width() * p as f32;
            ui.painter().line_segment(
                [pos2(x, rect.top()), pos2(x + 40.0, rect.top())],
                egui::Stroke::new(theme::HAIRLINE, t.accent),
            );
        }
    }

    match list {
        ListState::ReadFailed(cause) => {
            ui.add_space(16.0);
            ui.label(egui::RichText::new(format!("! {}", s.read_failed)).color(t.err));
            ui.label(egui::RichText::new(cause).font(crate::fonts::font_id(11.0, true)).color(t.ink2));
            ui.label(egui::RichText::new(s.read_failed_next).color(t.ink2).small());
        }
        ListState::Empty => {
            ui.add_space(16.0);
            ui.label(egui::RichText::new(s.empty_dir).color(t.ink2));
            ui.label(egui::RichText::new(s.empty_dir_next).color(t.ink2).small());
        }
        ListState::Ok => {
            egui::ScrollArea::vertical().show(ui, |ui| {
                header_row(ui);
                for (i, f) in files.iter().enumerate() {
                    let sel = i == *selected;
                    let (rect, resp) = ui.allocate_exact_size(
                        egui::vec2(ui.available_width(), 22.0),
                        Sense::click(),
                    );
                    let resp = resp.on_hover_cursor(egui::CursorIcon::PointingHand);
                    if resp.clicked() {
                        *selected = i;
                    }
                    if resp.double_clicked() {
                        *selected = i;
                        action = Action::Activate(i);
                    }
                    let painter = ui.painter_at(rect);
                    if sel {
                        painter.rect_filled(rect, 2.0, theme::surface_hover());
                        painter.rect_filled(
                            egui::Rect::from_min_size(
                                pos2(rect.left(), rect.top()),
                                egui::vec2(2.0, rect.height()),
                            ),
                            0.0,
                            t.accent,
                        );
                    }
                    row_text(&painter, rect, f, sel);
                }
            });
            ui.add_space(2.0);
            ui.label(egui::RichText::new(s.hint).small().color(t.ink2));
        }
    }
    action
}

fn small_tool(ui: &mut egui::Ui, label: &str, tip: &str) -> egui::Response {
    ui.button(label)
        .on_hover_text(tip)
        .on_hover_cursor(egui::CursorIcon::PointingHand)
}

fn header_row(ui: &mut egui::Ui) {
    let t = theme::theme();
    let (rect, _) = ui.allocate_exact_size(
        egui::vec2(ui.available_width(), 16.0),
        Sense::hover(),
    );
    let p = ui.painter_at(rect);
    for (x, txt) in [(12.0f32, "#"), (300.0, "size"), (360.0, "gen"), (410.0, "mtime")] {
        p.text(
            pos2(rect.left() + x, rect.center().y),
            Align2::LEFT_CENTER,
            txt,
            crate::fonts::font_id(crate::fonts::SZ_DIM, true),
            t.accent,
        );
    }
}

fn row_text(painter: &egui::Painter, rect: egui::Rect, f: &FileEntry, sel: bool) {
    let t = theme::theme();
    let color = if sel {
        t.ink
    } else if f.is_dir {
        t.ink2
    } else if f.is_dna {
        t.accent2
    } else {
        t.ink2
    };
    let name = if f.is_dir { format!("{}/", f.name) } else { f.name.clone() };
    let font = crate::fonts::font_id(crate::fonts::SZ_BODY, true);
    painter.text(pos2(rect.left() + 12.0, rect.center().y), Align2::LEFT_CENTER, &name, font.clone(), color);
    let size = if f.is_dir { "-".into() } else { human(f.size) };
    painter.text(pos2(rect.left() + 300.0, rect.center().y), Align2::LEFT_CENTER, &size, font.clone(), color);
    let gen = if f.is_dir {
        "-".into()
    } else {
        f.generation.map(|g| g.to_string()).unwrap_or_else(|| "-".into())
    };
    painter.text(pos2(rect.left() + 360.0, rect.center().y), Align2::LEFT_CENTER, &gen, font.clone(), color);
    painter.text(
        pos2(rect.left() + 410.0, rect.center().y),
        Align2::LEFT_CENTER,
        &f.mtime.map(|t| t.to_string()).unwrap_or_default(),
        font,
        color,
    );
}

#[cfg(test)]
mod tests {
    use super::*;

    /* a folder of only subfolders must not read as "no files" */
    #[test]
    fn scan_sees_directories_first() {
        let root = std::env::temp_dir().join(format!("hx_scan_{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        std::fs::create_dir_all(root.join("beta")).unwrap();
        std::fs::create_dir_all(root.join("alpha")).unwrap();
        std::fs::write(root.join("x.txt"), b"hi").unwrap();
        let entries = scan(&root).unwrap();
        assert_eq!(entries.len(), 3);
        assert!(entries[0].is_dir && entries[1].is_dir);
        assert_eq!(entries[0].name, "alpha");
        assert!(!entries[2].is_dir);
        let _ = std::fs::remove_dir_all(&root);
    }

    #[test]
    fn scan_reports_read_failure_with_cause() {
        let bogus = std::path::Path::new("/proc/helix-definitely-missing-42");
        assert!(scan(bogus).is_err());
    }
}
