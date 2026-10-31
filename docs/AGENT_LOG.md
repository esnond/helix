
A14 (2026-10-31): concurrent second agent session verified (site/ +
DESIGN_WORK_PALETTE.md). palette conflict resolved: design_decision.md
(owner-approved paper + crimson) binds; WORK_PALETTE marked as
proposal, engineering rules folded in. eframe 0.31 migration notes:
App::on_close_event -> on_exit(Option<&glow::Context>), FontData -> Arc,
Margin i8, Frame::NONE, NativeOptions.viewport, android entry =
NativeOptions.android_app. apksigner silent failures were /tmp exhaustion
(99%); workaround JAVA_TOOL_OPTIONS=-Djava.io.tmpdir=$HOME/tmp.
