//! job queue: one crypto operation at a time on a worker thread. the c
//! call is uninterruptible (~3.4 s), so cancel marks the row and drops a
//! late result; the thread itself finishes naturally.

use std::sync::mpsc;
use std::sync::Arc;

#[derive(Clone)]
pub enum Op {
    Encrypt { inp: String, out: String, m: String, gen: u32 },
    Decrypt { inp: String, out: String, m: String },
    Evolve { m: String },
    List { m: String },
}

impl Op {
    pub fn label(&self, s: &crate::i18n::Strings) -> String {
        match self {
            Op::Encrypt { inp, .. } => format!("{} → {inp}", s.encrypt),
            Op::Decrypt { inp, .. } => format!("{} ← {inp}", s.decrypt),
            Op::Evolve { .. } => s.evolve.to_string(),
            Op::List { .. } => s.reload.to_string(),
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum JobState {
    Running,
    Cancelling,
    Done,
    Failed,
    Cancelled,
}



pub struct JobRow {
    pub id: u64,
    pub op: Op,
    pub state: JobState,
    pub message: Option<String>,
    cancel: Arc<std::sync::atomic::AtomicBool>,
}

/// what a finished job hands back to the ui: a journal line and, for
/// chain operations, the refreshed key list
#[derive(Clone)]
pub struct JobOutput {
    pub message: String,
    pub keys: Option<crate::wrapper::KeyList>,
}

enum Event {
    Finished(u64, Result<JobOutput, String>),
    Cancelled(u64),
}

pub struct Queue {
    rows: Vec<JobRow>,
    tx: mpsc::Sender<Event>,
    rx: mpsc::Receiver<Event>,
    next_id: u64,
    /// completion flashes for the nacre sweep
    pub success_at: Option<f64>,
    pub failure_at: Option<f64>,
}

impl Default for Queue {
    fn default() -> Self {
        let (tx, rx) = mpsc::channel();
        Queue {
            rows: Vec::new(),
            tx,
            rx,
            next_id: 1,
            success_at: None,
            failure_at: None,
        }
    }
}

impl Queue {
    pub fn is_busy(&self) -> bool {
        self.rows.iter().any(|r| matches!(r.state, JobState::Running | JobState::Cancelling))
    }

    pub fn rows(&self) -> &[JobRow] {
        &self.rows
    }

    /// spawn the op on a worker thread; the caller must not push while
    /// the queue is busy (the ui keeps one active crypto job)
    pub fn push(&mut self, op: Op) -> u64 {
        let id = self.next_id;
        self.next_id += 1;
        let cancel = Arc::new(std::sync::atomic::AtomicBool::new(false));
        let cancel2 = cancel.clone();
        let tx = self.tx.clone();
        self.rows.push(JobRow {
            id,
            op: op.clone(),
            state: JobState::Running,
            message: None,
            cancel,
        });
        std::thread::spawn(move || {
            let res = run_op(&op, &cancel2);
            if cancel2.load(std::sync::atomic::Ordering::Relaxed) {
                let _ = tx.send(Event::Cancelled(id));
            } else {
                let _ = tx.send(Event::Finished(id, res));
            }
        });
        id
    }

    pub fn cancel(&mut self, id: u64) {
        for r in self.rows.iter_mut() {
            if r.id == id && matches!(r.state, JobState::Running) {
                r.cancel.store(true, std::sync::atomic::Ordering::Relaxed);
                r.state = JobState::Cancelling;
            }
        }
    }

    /// drain worker events; returns (finished op, key list if any)
    pub fn poll(&mut self) -> Option<(Op, JobOutput)> {
        let mut events = Vec::new();
        while let Ok(ev) = self.rx.try_recv() {
            events.push(ev);
        }
        let mut finished = None;
        for ev in events {
            match ev {
                Event::Finished(id, res) => {
                    if let Some(r) = self.rows.iter_mut().find(|r| r.id == id) {
                        match res {
                            Ok(out) => {
                                r.state = JobState::Done;
                                r.message = Some(out.message.clone());
                                finished = Some((r.op.clone(), out));
                            }
                            Err(e) => {
                                r.state = JobState::Failed;
                                r.message = Some(e);
                            }
                        }
                    }
                    match &finished {
                        Some(_) => self.success_at = Some(now_s()),
                        None => self.failure_at = Some(now_s()),
                    }
                }
                Event::Cancelled(id) => {
                    for r in self.rows.iter_mut() {
                        if r.id == id {
                            r.state = JobState::Cancelled;
                            r.message = Some("cancelled".into());
                        }
                    }
                }
            }
        }
        /* keep the history bounded: the newest rows stay visible */
        if self.rows.len() > 8 {
            self.rows.drain(0..self.rows.len() - 8);
        }
        finished
    }
}

fn now_s() -> f64 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs_f64())
        .unwrap_or(0.0)
}

fn run_op(
    op: &Op, cancel: &std::sync::atomic::AtomicBool,
) -> Result<JobOutput, String> {
    use crate::wrapper;
    match op {
        Op::Encrypt { inp, out, m, gen } => {
            let r = wrapper::encrypt(inp, out, m, None, *gen);
            check_cancel(cancel)?;
            r.map(|_| JobOutput { message: format!("{inp} → {out}"), keys: None })
        }
        Op::Decrypt { inp, out, m } => {
            let r = wrapper::decrypt(inp, out, m, None);
            check_cancel(cancel)?;
            r.map(|_| JobOutput { message: format!("{inp} → {out}"), keys: None })
        }
        Op::Evolve { m } => {
            let r = wrapper::evolve(m, None);
            check_cancel(cancel)?;
            let list = wrapper::list_generations(m, None).ok();
            r.map(|(g, _)| JobOutput {
                message: format!("gen {g}"),
                keys: list,
            })
        }
        Op::List { m } => {
            let r = wrapper::list_generations(m, None);
            check_cancel(cancel)?;
            r.map(|l| JobOutput {
                message: format!("{} gens", l.keys.len()),
                keys: Some(l),
            })
        }
    }
}

fn check_cancel(cancel: &std::sync::atomic::AtomicBool) -> Result<(), String> {
    if cancel.load(std::sync::atomic::Ordering::Relaxed) {
        Err("cancelled".into())
    } else {
        Ok(())
    }
}
