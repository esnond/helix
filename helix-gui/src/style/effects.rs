//! lain style overlays — scanlines, vignette, noise, glow, flicker, blink, print, glitch.
//! all cheap painter tricks, no shaders, single overlay pass where possible.

use eframe::egui::{self, Color32, Context, LayerId, Order, Rounding, Stroke, Rect, Painter};

use crate::style::colors::*;

/// faint horizontal scanlines across the whole screen — 1px every 3px at 12% alpha
pub fn scanlines(ctx: &Context) {
    let screen = ctx.input(|i| i.screen_rect);
    let painter =
        ctx.layer_painter(LayerId::new(Order::Foreground, egui::Id::new("scanlines")));
    let mut y = screen.top();
    while y < screen.bottom() {
        painter.line_segment(
            [egui::pos2(screen.left(), y), egui::pos2(screen.right(), y)],
            Stroke::new(1.0, scanline_color()),
        );
        y += 3.0;
    }
}

/// radial vignette — dark gradient at corners, 0% center to 35% edges
pub fn vignette(ctx: &Context) {
    let screen = ctx.input(|i| i.screen_rect);
    let painter = ctx.layer_painter(LayerId::new(Order::Foreground, egui::Id::new("vignette")));
    
    // draw concentric rects for vignette effect
    for i in 1..=8 {
        let t = i as f32 / 8.0;
        let alpha = (0.35 * t * t) as u8; // quadratic falloff
        let rect = Rect::from_min_max(
            screen.min + egui::vec2(i as f32 * 8.0, i as f32 * 8.0),
            screen.max - egui::vec2(i as f32 * 8.0, i as f32 * 8.0),
        );
        painter.rect(
            rect,
            Rounding::same(0.0),
            Color32::TRANSPARENT,
            Stroke::new(2.0, theme().bg.gamma_multiply((alpha as f32 / 255.0).min(1.0))),
        );
    }
}

/// noise grain overlay — monochrome static at 3% alpha, animated at 12fps
pub fn noise_grain(ctx: &Context) {
    let screen = ctx.input(|i| i.screen_rect);
    let painter = ctx.layer_painter(LayerId::new(Order::Foreground, egui::Id::new("noise")));
    
    // animate noise at ~12fps by using frame time
    let frame = (ctx.input(|i| i.time) * 12.0).floor() as u64;
    
    // simple deterministic noise pattern based on frame
    let seed = frame.wrapping_mul(0x9e3779b9);
    let mut rng_state = seed;
    
    let w = screen.width() as i32;
    let h = screen.height() as i32;
    let stride = ((w + 3) / 4) * 4; // align to 4 for efficiency
    
    let mut noise_data = Vec::with_capacity((stride * h) as usize);
    
    for _y in 0..h {
        for _x in 0..w {
            rng_state = rng_state.wrapping_mul(0x9e3779b9).wrapping_add(1);
            let val = ((rng_state >> 16) & 0xFF) as u8;
            let alpha = if val < 77 { 12 } else { 0 }; // ~3% density
            noise_data.push(alpha);
        }
    }
    
    // draw sparse noise points - skip every other pixel for performance
    let mut y = 0;
    while y < h {
        let mut x = 0;
        while x < w {
            let idx = (y * stride + x) as usize;
            if idx < noise_data.len() && noise_data[idx] > 0 {
                painter.rect_filled(
                    Rect::from_min_size(
                        egui::pos2(screen.left() + x as f32, screen.top() + y as f32),
                        egui::vec2(1.0, 1.0),
                    ),
                    Rounding::ZERO,
                    noise_grain_color(),
                );
            }
            x += 2; // skip every other pixel for performance
        }
        y += 2;
    }
}

/// CRT flicker — global brightness oscillates 98%-100% at 0.5Hz
pub fn crt_flicker(ctx: &Context) {
    let _screen = ctx.input(|i| i.screen_rect);
    let painter = ctx.layer_painter(LayerId::new(Order::Foreground, egui::Id::new("crt_flicker")));
    
    let t = ctx.input(|i| i.time);
    let flicker = (t * std::f64::consts::PI).sin() as f32 * 0.01; // 0.5Hz, ±1%
    let alpha = (flicker * 255.0).abs() as u8;
    
    if alpha > 0 {
        painter.rect_filled(
            ctx.input(|i| i.screen_rect),
            Rounding::ZERO,
            Color32::from_rgba_unmultiplied(0x07, 0x07, 0x0f, alpha.min(12)),
        );
    }
}

/// cursor blink — block cursor blinks 1s on / 1s off
pub struct CursorBlink {
    pub active: bool,
}

impl CursorBlink {
    pub fn new() -> Self {
        CursorBlink { active: true }
    }
    
    pub fn update(&mut self, ctx: &Context) {
        let t = ctx.input(|i| i.time);
        self.active = (t * 1.0).sin() > 0.0; // 1s period
    }
    
    pub fn is_active(&self) -> bool {
        self.active
    }
}

/// text print effect — types label char by char over 180ms
pub struct TextPrint {
    pub target: String,
    pub current: String,
    pub start_time: Option<f64>,
    pub duration: f64,
}

impl TextPrint {
    pub fn new(target: String, duration: f64) -> Self {
        TextPrint {
            target,
            current: String::new(),
            start_time: None,
            duration,
        }
    }
    
    pub fn trigger(&mut self, now: f64) {
        self.start_time = Some(now);
        self.current.clear();
    }
    
    pub fn update(&mut self, now: f64) -> String {
        if let Some(start) = self.start_time {
            let elapsed = now - start;
            if elapsed >= self.duration {
                self.current = self.target.clone();
            } else {
                let progress = (elapsed / self.duration).min(1.0);
                let chars = (self.target.len() as f64 * progress) as usize;
                self.current = self.target.chars().take(chars).collect();
            }
        }
        self.current.clone()
    }
    
    pub fn is_done(&self) -> bool {
        self.current == self.target
    }
    
    pub fn reset(&mut self, target: String) {
        self.target = target;
        self.current.clear();
        self.start_time = None;
    }
}

/// glitch on error — 120ms horizontal slice displacement + color flash + rgb split
pub struct Glitch {
    pub until: f64,
}

impl Glitch {
    pub fn trigger(now: f64) -> Self {
        Glitch { until: now + 0.12 }
    }
    
    pub fn active(&self, now: f64) -> bool {
        now < self.until
    }
    
    /// x offset in points, pseudo random per frame
    pub fn offset(&self, now: f64) -> f32 {
        ((now * 60.0).sin() * 4.0) as f32
    }
    
    /// rgb split offset for red channel
    pub fn red_offset(&self, now: f64) -> f32 {
        ((now * 80.0).cos() * 1.5) as f32
    }
    
    /// blue channel offset
    pub fn blue_offset(&self, now: f64) -> f32 {
        ((now * 70.0).sin() * 1.5) as f32
    }
}

/// glow focus — focused panel gets soft box-shadow
pub fn glow_focus(painter: &Painter, rect: Rect, color: Color32, strength: f32) {
    if strength <= 0.0 {
        return;
    }
    for i in 1..=4 {
        let a = (40.0 * strength / i as f32) as u8;
        painter.rect(
            rect.expand(i as f32 * 3.0),
            Rounding::same(3.0),
            Color32::TRANSPARENT,
            Stroke::new(1.5, Color32::from_rgba_unmultiplied(color.r(), color.g(), color.b(), a)),
        );
    }
}

/// draw a terminal-style prompt button ("> label")
pub fn draw_prompt_button(
    ui: &mut egui::Ui,
    label: &str,
    hovered: bool,
    _color: Color32,
) -> egui::Response {
    let text = format!("> {}", label);
    let color = if hovered { theme().accent } else { theme().ink };
    
    ui.add(egui::Button::new(
        egui::RichText::new(text)
            .font(crate::style::fonts::font_id(13.0))
            .color(color)
    ).fill(egui::Color32::TRANSPARENT).stroke(egui::Stroke::NONE))
}

/// draw a selection row with left accent bar (lain cursor style)
pub fn draw_selection_row(
    painter: &Painter,
    rect: Rect,
    selected: bool,
    _accent_color: Color32,
    _strength: f32,
) {
    if !selected {
        return;
    }
    
    // background at 18% alpha
    painter.rect_filled(
        rect,
        Rounding::ZERO,
        Color32::from_rgba_unmultiplied(
            theme().accent.r(),
            theme().accent.g(),
            theme().accent.b(),
            46, // 18% of 255
        ),
    );
    
    // left 2px solid accent bar
    painter.rect_filled(
        Rect::from_min_size(
            rect.min,
            egui::vec2(2.0, rect.height()),
        ),
        Rounding::ZERO,
        theme().accent,
    );
}