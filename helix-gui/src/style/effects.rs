//! lain style overlays: scanlines and a soft glow on the focused
//! element. cheap painter tricks, no shaders.

use eframe::egui::{self, Color32, Context, LayerId, Order, Rounding, Stroke};

/// faint horizontal scanlines across the whole screen
pub fn scanlines(ctx: &Context) {
    let screen = ctx.input(|i| i.screen_rect);
    let painter =
        ctx.layer_painter(LayerId::new(Order::Foreground, egui::Id::new("scanlines")));
    let mut y = screen.top();
    while y < screen.bottom() {
        painter.line_segment(
            [egui::pos2(screen.left(), y), egui::pos2(screen.right(), y)],
            Stroke::new(1.0, Color32::from_rgba_unmultiplied(0, 0, 0, 14)),
        );
        y += 3.0;
    }
}

/// translucent glow strokes around a focused rect
pub fn glow(painter: &egui::Painter, rect: egui::Rect, color: Color32, strength: f32) {
    if strength <= 0.0 {
        return;
    }
    for i in 1..=3 {
        let a = (26.0 * strength / i as f32) as u8;
        painter.rect(
            rect.expand(i as f32 * 2.5),
            Rounding::same(3.0),
            Color32::TRANSPARENT,
            Stroke::new(1.5, Color32::from_rgba_unmultiplied(color.r(), color.g(), color.b(), a)),
        );
    }
}

/// brief lain glitch on errors: the caller draws text twice with a
/// jitter while the flash is alive
pub struct Glitch {
    pub until: f64,
}

impl Glitch {
    pub fn trigger(now: f64) -> Self {
        Glitch { until: now + 0.1 }
    }
    pub fn active(&self, now: f64) -> bool {
        now < self.until
    }
    /// x offset in points, pseudo random per frame
    pub fn offset(&self, now: f64) -> f32 {
        ((now * 60.0).sin() * 2.5) as f32
    }
}
