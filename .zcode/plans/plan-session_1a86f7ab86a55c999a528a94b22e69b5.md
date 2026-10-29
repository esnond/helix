## helix → v1.0: полный план (GUI с нуля + закрытие всех крипто-дыр)

Решения зафиксированы: полный объём (включая .dna v3 с внешней kyber-парой и argon2id), GUI переписывается в том же `helix-gui/` (старый код остаётся в git-истории). Все дыры из моего списка закрыть.

### Каркас исполнения (каждый этап)
- Ветка `v10-run` от main, якорь-тег `v1.3-anchor`, `docs/STATE.md` — чекпоинт после каждого этапа.
- Зелёные гейты перед переходом: `make test` + `make test-san` + `make gate-libm` + `cargo test`. Перед каждым коммитом — /gate по приоритету скиллов.
- Агенты строго поочерёдно (R1): architect → реализация inline → methodologist (клаймы) или adversary → blue_team (верификация фиксов прогоном) → memory (запись решения) → editor (доки этапа). Сбой спавна = запись в AGENT_LOG и продолжение без агента.
- Токен-экономия: шумные поиски — только через Explore-агента; чтение файлов @-ссылками и sed-диапазонами (никаких cat >150 строк); stdout >20 строк → `results/*.log` + tail в контекст; /compact каждые 20–30 ходов; отчёты субагентов ≤900 слов; Memory OFF.

### Этап 1 — GUI с нуля по референсу (главная боль)
`git rm -r helix-gui/src` и новый код в тот же путь. Источник истины: `docs/design/constitution.md` + `docs/design/v5_reference.html` (не буквальный порт).
- Стек: eframe/egui 0.23 (проверенные API), cpal для звука, один FFI-модуль `gui_api.c` — единственная unsafe-граница, clippy -D warnings.
- Модули: `app.rs` (состояние + actions), `panel_files.rs`, `panel_keys.rs`, `panel_jobs.rs`, `bar_status.rs`, `dialog.rs` (одна модалка: seed/encrypt/decrypt/delete/keypair), `theme.rs` (обе полярности), `sound/{engine,ir,sources}.rs`, `i18n.rs` (полный словарь RU/EN, включая ошибки диалогов), `prefs.rs` (persist: полярность, язык, звук, размер окна, последний каталог).
- Приёмка (всё мышью, без клавиатуры): клик выбирает, двойной клик открывает, вход/выход из папок, encrypt/decrypt/delete, ввод фразы, очередь задач со строками (причина, прогресс-волосок, cancel), dendrite с тултипом честной цены пересчёта, пустой каталог и ошибка чтения с причиной + одним действием, nacre-свип по завершении, help как два статических блока.
- Звук по конституции §8: bus → convolver (процедурный metal-box IR, wet 0.13) → компрессор → lowpass 14k; источники: контакт-тик (шум+HP), негармоничная пластина (моды 1:2.76:5.4:8.9, поканальный decay, джиттер), глухой thump +6мс, вторичный контакт +42мс±джиттер, линия = свип bandpass-шума с пакетами, гул катушки 50/53.9Гц пока жива очередь, fault = реле AM 100Гц на низкой пиле; джиттер = hash(режим+имя+bucket); по умолчанию ВЫКЛ, у каждого звука визуальный дубль. Риск: cpal на wayland — fallback null-бэкенд.
- Хард-правила: никакой крипто-логики в Rust — только вызовы FFI; Ctrl+Q; ни одного bare spinner.
- Гейты: cargo test (prefs roundtrip, полнота i18n-словаря, граф звука), запуск из $HOME, чеклист `docs/gui_acceptance.md` (~20 пунктов, отмечаешь руками).
- Агенты: architect (каркас) → реализация → methodologist (приёмка против конституции) → adversary (UI-поверхность: путь в delete, имена-инъекции, symlink) → memory → editor (r5_spec v2, README).

### Этап 2 — надёжное затирание секретов (дыра 4)
`src/secure_mem.{h,c}`: `explicit_bzero` (POSIX) + `mlock` для материала ключей; замена memset по kdf/evolve/keycheck/aead/kyber_wrap/dna/kyber_drbg; канареечный тест. Остаточный риск (компилятор/своп) — честно в limitations. c99-posix-strict: POSIX-заголовки допустимы. blue_team верифицирует отсутствие регресса тестами.

### Этап 3 — фаззер парсера .dna (дыра 1)
`tools/fuzz_dna.c`: детерминированный мутационный фуззинг `dna_parse_header`+`dna_decrypt_file` (буферы ≤4КБ, 10 млн execs через nohup, night-shift-ops), краши → `results/fuzz_dna/`. Прогон под ASAN. Отчёт: execs, находки, вердикты (statistical-rigor). adversarial-скилл: каждую находку либо фиксим, либо рефутируем данными.

### Этап 4 — .dna v3: внешняя kyber-пара + argon2id (полный объём)
- `kdf_type=2`: сеансовый ключ = sha256(外部 kyber_ss ‖ chain_key ‖ tag), kyber-пара живёт в файле ключей (`.hkpair`: открытый/закрытый ключ, зашифрованный фразой), заголовок несёт ct. Впервые реальная PQC-ценность: фраза не нужна для расшифровки, нужен файл пары.
- argon2id: опция в заголовке (поле iterations переиспользуем как селектор профиля), реализация своя по RFC 9106, KAT из векторов RFC, флаг CLI `--kdf argon2id|pbkdf2`.
- Обратная совместимость: v2 и v1(ошибка с внятным текстом) читаются; v3 пишется по умолчанию только с внешней парой, без пары — v2.
- Тесты: KAT argon2id, roundtrip v3, tamper, миграционные кейсы. methodologist регистрирует гипотезы, adversary ред-тимит формат, blue_team верифицирует.
- GUI (добавка к этапу 1): секция «пара ключей» — создать/импортировать/экспортировать, диалог encrypt с выбором «фраза | файл пары».

### Этап 5 — Keychain KCH2 (дыра 5)
Шифруемый кейчейн: magic `KCH2`, aead(chacha20poly1305) всех поколений ключом sha256(mk‖"helix-kch"), tamper-evidence, блокировка flock. Миграция KCH1→KCH2 при первом evolve (флаг `--migrate-kch`), KCH1 только читается. Тесты: tamper, flock, миграция. adversary → blue_team.

### Этап 6 — cross-libc + arm64 (дыра 3)
Ты ставишь тулчейны (sudo, пароль твой): `sudo apt install musl-tools gcc-aarch64-linux-gnu qemu-user`. Я: `scripts/cross_libc.sh` (готов) + новый `scripts/cross_arch.sh` (aarch64 под qemu-user-static) → таблица в `results/cross_platform/`. Несовпадение ключа = блокер релиза с разбором, не отмашка.

### Этап 7 — релиз v1.0
editor: limitations, security_claims, `docs/r6_spec.md` (GUI+v3+KCH2), README final, blog FINAL, `RELEASE_v1.0.md`. reproducibility-manifest: манифест релиза (binary sha, компилятор, флаги, сиды). Тег `v1.0-rc` → твоя ручная приёмка чеклиста → `v1.0`. Пуш — твой, `./scripts/push_run.sh` обновлю под новые теги.

### Порядок и деградация при нехватке токенов
1 GUI → 2 zeroize → 3 фаззер → 4 v3+argon2 → 5 KCH2 → 6 cross → 7 релиз. Стоп только на чекпоинте STATE.md, никогда посреди этапа. Отложенное и не сделанное — в roadmap с пометкой, никаких «тихих» урезаний.