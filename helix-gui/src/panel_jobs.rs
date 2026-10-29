//! the jobs strip: one line per queued or finished operation with a
//! progress hairline, a reason line and a cancel button. no bare
//! spinners anywhere.

use eframe::egui::{self, Sense};

use crate::i18n;
use crate::jobs::{JobState, Queue};
use crate::theme;

pub fn show(ui: &mut egui::Ui, queue: &Queue, now: f64) -> Option<u64> {
    let t = theme::theme();
    let s = i18n::s();
    let mut cancel_id = None;

    ui.horizontal(|ui| {
        ui.label(theme::micro_head(s.jobs));
        ui.add_space(12.0);
        if queue.rows().is_empty() {
            ui.label(egui::RichText::new(s.no_jobs).small().color(t.ink2));
            return;
        }
        for row in queue.rows().iter().rev().take(3) {
            let (color, mark) = match row.state {
                JobState::Running => (t.accent, "▸"),
                JobState::Cancelling => (t.warn, "×"),
                JobState::Done => (t.ok, "✓"),
                JobState::Failed => (t.err, "!"),
                JobState::Cancelled => (t.ink2, "×"),
            };
            ui.label(egui::RichText::new(mark).color(color));
            ui.label(
                egui::RichText::new(row.op.label(s))
                    .font(crate::fonts::font_id(12.0, true))
                    .color(if matches!(row.state, JobState::Running | JobState::Cancelling) {
                        t.ink
                    } else {
                        t.ink2
                    }),
            );
            if let Some(msg) = &row.message {
                if !matches!(row.state, JobState::Running | JobState::Cancelling) {
                    ui.label(egui::RichText::new(msg).small().color(color));
                }
            }
            if matches!(row.state, JobState::Running | JobState::Cancelling) {
                /* indeterminate hairline: the c call has no progress */
                let (rect, _) =
                    ui.allocate_exact_size(egui::vec2(120.0, 2.0), Sense::hover());
                let phase = ((now * 0.6) as f32).fract();
                let w = rect.width() * 0.35;
                let x = rect.left() + (rect.width() - w) * phase;
                ui.painter().rect_filled(
                    egui::Rect::from_min_size(egui::pos2(x, rect.top()), egui::vec2(w, 2.0)),
                    0.0,
                    color,
                );
                if matches!(row.state, JobState::Running) && theme::btn(ui, s.cancel, false).clicked() {
                    cancel_id = Some(row.id);
                }
            }
            ui.add_space(8.0);
        }
    });
    cancel_id
}
