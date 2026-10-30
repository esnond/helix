# план r8: закрытие дыр (2026-10-31)

авторы: inline-ресёрч (architect/adversary-спавны заблокированы провайдером
второй раз, AGENT_LOG A13). все утверждения о коде проверены чтением
исходников; оценки усилий: S < 2ч, M < день, L > день.

## факт, меняющий приоритеты

keychain is a cache, not a source of truth. `helix_generation_key(mk, NULL,
gen)` (src/kdf.c:288) пересчитывает любое поколение из мнемоники: gen0 =
`helix_engine_phase(mk)`, gen_n = цепочка `evolution_recombine` (чистый
sha256, src/evolve.c:28, без энтропии). следствия:

- потеря/расхождение кейчейна стоит только CPU: пересчёт ≈ 3.4 с + мс на
  recombine на поколение;
- протокол merge двух кейчейнов НЕ нужен. синхронизация = «пере-выведи
  цепочку на втором устройстве» + опциональный экспорт файла кейчейна;
- honest statement для док: «кейчейн — кэш и курсор; вся истина в
  мнемонике» (уже в r1_security.md claim 2, теперь операционализируем).

## этапы (порядок жёсткий: 1 → 2 → 3; параллелить 4-7)

### этап 1. android рендер (P0, M)
- апгрейд helix-gui: eframe 0.23 → 0.28 (egui 0.28, winit 0.29 — нативный
  android-activity, `eframe::run_native` работает на android без кастомных
  мостков).
- известные API-разрывы 0.23→0.28 (все уже встречены в 0.23-коде):
  NativeOptions.initial_window_size/fullscreen → viewport builder;
  egui::Image::new(handle, size) → ImageSource + fit_to_exact_size;
  TextureOptions поля; Margin i8↔f32; ctx.copy_text; theme setter.
  оценка: механическая правка ~6 файлов (theme, dialog, panels, pixel,
  mobile, main) + cargo test 8/8 как гейт.
- helix-android/src/lib.rs: вернуть eframe::run_native внутри
  android_main с android-activity app handle (winit 0.29 сам подхватит
  native-activity из манифеста cargo-apk).
- гейт: cargo check desktop + android; cargo test 8/8; apk ставится,
  UI виден, v2 roundtrip на устройстве.
- риск: eframe 0.28 android-activity version vs cargo-apk 0.9 (ожидает
  ndk-glue) — возможно понадобится cargo-apk ≥ 0.9.4 или xtask-сборка.
  fallback: оставить cargo-apk, манифест генерить руками (aapt2).

### этап 2. выпуск (P0, S)
- коммит working tree (уже сделан, 05b82d6).
- docs/design/design_decision.md: отвергнутые макеты (v5 brass,
  «операторская», aqua) → принятый (1-bit mac refined + лого А + маскот),
  с причинами из сессии.
- приёмка: gui_acceptance.md 35 пунктов руками owner (после этапа 1,
  чтобы гонять один бинарник).
- тег v1.0 → push: main ff-only от r2, теги v1.0-rc + v1.0 (owner, PAT).

### этап 3. android-специфика (P1, S/M)
- kch.db → внутренняя память (getFilesDir), файлы остаются во external:
  внешний kch.db безопасен и так (AEAD под sha256(mk||tag), утечка даёт
  только шифротекст + 8-байтный fp), но внутренняя память снимает вопрос
  целиком. правка одной строки в mobile.rs (kch()).
- clipboard: кнопка «показать» вместо автокопирования; копирование по
  явному тапу с тостом-предупреждением; экран фразы/ключа под
  FLAG_SECURE (jni setWindowFlags при открытии диалога, S на android).
- SAF-пикер: roadmap v1.1 (M) — читаемое решение уже есть через
  adb/mtp; не блокирует.
- звук android: off по умолчанию и сейчас; cpal obackend — M, не
  блокирует.

### этап 4. инфраструктура (P1, S)
- GitHub Actions: job desktop (make test, make test-san, cargo test),
  job android (fat apk, artifact на тег). раннер: ubuntu-latest + ndk
  25.2 + openjdk-17 + cargo-apk; ~20 мин/прогон.Effort S.
- scripts/build_android.sh дописывает results/release/android_manifest.txt:
  sha256 lib*.so по ABI, rustc -vV, ndk-версия, флаги. гейт
  reproducibility-manifest. Effort S.

### этап 5. streaming .dna (P2, S/M)
- решение: mmap (nix::mmap) чтения и записи, формат НЕ меняется —
  один AEAD-рекорд на весь файл; память = page cache, не RSS. armv7
  (32-бит) тянет до ~2 ГБ файлов. чанкование отложено до потребности
  >2 ГБ: чанки раскрывают границы и требуют новый format version.
- гейт: roundtrip 1 ГБ файла, сравнение sha с desktop-путём.

### этап 6. измерения безопасности (P2, M/L)
- cache side-channel evolve.c: флаш-перезагрузочный прогон по методике
  results/side_channel_audit/ (расширение dudect на aarch64 под qemu
  не даёт кэш-реальности; нужен нативный прогон на устройстве —
  скрипт-харнесс в репо, запуск руками на телефоне через adb). M.
- aarch64 нативные гейты: asan/ubsan под qemu уже есть; dudect aarch64
  добавить в scripts/cross_arch.sh. S.
- third-party audit: вне зоны проекта; подготовить пакет для аудитора
  (threat model + claims + фаззер-отчёты) — S, сам аудит внешнее.

### этап 7. pbkdf2→argon2id в derivation (P2, решение owner)
- смена форкает все кейчейны: генерация v1 ключей потребует
  dual-read (старый кейчейн) и миграцию. рекомендация: НЕ менять до
  реальной угрозы GPU-брутфорса; документировать как осознанный
  компромисс в security_claims.md (claim 8 уже честен).

## порядок исполнения

этап 1 (рендер) → этап 3 (android-гигиена) → этап 4 (CI+manifest)
→ этап 2 (приёмка+тег+пуш) → 5 → 6 → 7. этапы 5-7 не блокируют v1.0.

## что НЕ входит

- ios-порт (нет тулчейна, отдельное решение);
- сетевые функции (r4 отменён, precondition'ы в r4_decision.md);
- merge-протокол кейчейнов (закрыт фактом re-derivation, этап 0).
