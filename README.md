# MHGU Overlay

A multilingual Monster Hunter Generations Ultimate overlay for Nintendo Switch, with live health, crown-aware size data, and experimental quest size presets.

[![build](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml/badge.svg)](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Features

- Compact in-game HUD for monster health, health percentage, size multiplier, actual size, crown class, and Hyper status.
- English, Simplified Chinese, and Japanese monster names and UI.
- Automatic language detection from the game's application control data. Unsupported languages and detection failures fall back to English.
- Preselect Mini, Silver, or Gold before entering a quest. When the monster object appears, the worker applies the corresponding legal crown threshold and verifies the value by reading it back.
- Portable C++ core with platform-specific memory, language, persistence, and UI adapters.
- Normalized data for 94 large monsters, generated from independent catalog and locale files.

The size-writing feature is experimental and disabled by default. Read [Size presets](#size-presets) before enabling it.

## Compatibility

| Target | Status |
| --- | --- |
| MHGU 1.4.0 on Atmosphère | Primary target; memory reads are implemented, hardware verification is still required |
| MHXX Nintendo Switch | Experimental; shares the current profile and defaults to Japanese |
| Ryujinx | This Tesla build does not run in Ryujinx; the portable core is designed for a future emulator adapter |
| Windows | Not included; a future adapter can reuse the core without depending on Tesla or libnx |

This repository is under active development. A successful CI build proves that the overlay compiles, not that every memory operation has been validated on every firmware, region, or game build.

## Requirements

- A Nintendo Switch running a current Atmosphère release.
- Tesla Menu / ovlmenu.
- Atmosphère's `dmnt:cht` service.
- MHGU 1.4.0 or MHXX for Nintendo Switch.

## Install

1. Download `mhgu-overlay.ovl` from the latest successful GitHub Actions artifact.
2. Copy it to:

   ```text
   sdmc:/switch/.overlays/mhgu-overlay.ovl
   ```

3. Start MHGU or MHXX.
4. Open Tesla Menu and select **MHGU Overlay**.
5. Enter a quest containing a large monster. The first scan may take a moment.

Settings are stored at:

```text
sdmc:/config/mhgu-overlay/settings.ini
```

## Usage

The main screen contains:

- **Open compact HUD**: switches to a transparent, low-refresh HUD and returns input focus to the game.
- **Language**: cycles through Auto, English, Simplified Chinese, and Japanese.
- **Quest size preset**: cycles through Off, Mini, Silver, and Gold.
- **Apply preset in quest**: arms or disarms experimental size writes.
- **Find monster list**: discards the cached in-memory pointer and scans again.

In compact HUD mode, hold the left and right sticks together to return to the settings screen.

## Size presets

Choose the preset and enable **Apply preset in quest** before entering the quest. The overlay does not try to edit a quest definition or save file in advance. Instead, it waits for each large monster's runtime object to be created, verifies the monster identity, writes the selected multiplier, and reads it back.

Safety rules in the current implementation:

- Off is the default.
- No write is attempted unless both a crown preset and the explicit lock toggle are enabled.
- Fixed-size monsters do not receive write requests.
- The target is derived from the catalog's legal Mini, Silver, or Gold threshold.
- The adapter verifies the monster identity immediately before writing.
- Only the size multiplier field is written.
- Values outside 50–200% are rejected.
- A successful write must pass an immediate read-back check.

Back up your save before testing. Crown registration, model scale, hitboxes, quest completion records, and special-event monsters still need systematic hardware validation. Do not assume this feature is safe for online or competitive use.

## Languages and data

Localization is intentionally separate from gameplay data:

```text
data/
├── catalog/          # IDs, size facts, crown thresholds, adapter aliases
├── locales/          # en, zh-Hans, ja
└── schema/           # validation contracts
```

To refresh public numeric facts and regenerate C++ tables:

```sh
python3 tools/refresh_catalog.py
python3 tools/validate_crowns.py
python3 tools/generate_catalog.py
make -f Makefile.host test
```

`refresh_catalog.py` imports names only for source-page verification and writes language-independent numeric data. It does not mirror page prose, quest tables, or media. The locale files are authoritative for all user-facing names and messages.

## Build

### Easiest path: GitHub Actions

You do not need to install C++ or the Switch toolchain for this path.

1. Push your branch to GitHub.
2. Open the repository's **Actions** tab.
3. Open the latest `build` workflow.
4. Wait for both `host-tests` and `switch` to turn green.
5. Download the `mhgu-overlay` artifact from the workflow summary.
6. Unzip it and copy `mhgu-overlay.ovl` to `sdmc:/switch/.overlays/`.

Every push and pull request runs the same checks automatically.

### Clone for development

This project uses two Git submodules. Always clone with `--recurse-submodules`:

```sh
git clone --recurse-submodules https://github.com/jinghaihan/mhgu-overlay.git
cd mhgu-overlay
```

If you already cloned without that option:

```sh
git submodule update --init --recursive
```

### Run tests without a Switch toolchain

The core and adapters have ordinary desktop tests. On macOS or Linux with a C++17 compiler:

```sh
make -f Makefile.host test
```

Expected output:

```text
core tests passed
catalog tests passed
switch adapter tests passed
settings tests passed
```

Use this command after changing core logic, data, translations, settings, or memory decoding.

### Compile with Docker

Docker is the simplest reproducible local Switch build because the image already contains devkitA64 and libnx:

```sh
docker run --rm \
  -v "$PWD:/project" \
  -w /project \
  devkitpro/devkita64:latest \
  make -j2
```

The result is `mhgu-overlay.ovl` in the repository root. Docker Desktop, OrbStack, or another Docker-compatible runtime must be running first.

### Compile with a local devkitPro installation

1. Install devkitPro and the `switch-dev` toolchain using the [official devkitPro setup guide](https://devkitpro.org/wiki/Getting_Started).
2. Confirm that the `DEVKITPRO` environment variable exists:

   ```sh
   echo "$DEVKITPRO"
   ```

3. Build:

   ```sh
   make -j2
   ```

4. Remove build output when you need a clean rebuild:

   ```sh
   make clean
   ```

Do not use `Makefile.host` to produce the Switch plugin. It only builds tests for the portable code.

## Development guide for beginners

You do not need to understand the whole project before making a small change.

### Where to make changes

| Goal | Edit |
| --- | --- |
| Change English UI text or monster names | `data/locales/en.json` |
| Change Simplified Chinese text or names | `data/locales/zh-Hans.json` |
| Change Japanese text or names | `data/locales/ja.json` |
| Change monster size/crown data | `data/catalog/monsters.seed.json` and the refresh pipeline |
| Change health, size, or preset decisions | `include/mhgu/core/` and `source/core/` |
| Change Switch addresses or title support | `source/platform/switch/game_profile.cpp` |
| Change memory validation/read/write behavior | `source/platform/switch/monster_reader.cpp` |
| Change the settings menu or compact HUD | `source/ui/main.cpp` |
| Change saved settings | `source/app/settings.cpp` |

Files under `source/generated/` are generated output. Do not edit them by hand.

### Normal edit-test-build loop

1. Create a branch:

   ```sh
   git switch -c feat/my-change
   ```

2. Edit the relevant source or data file.
3. If you changed a locale or catalog file, regenerate tables:

   ```sh
   python3 tools/generate_catalog.py
   ```

4. Run host tests:

   ```sh
   make -f Makefile.host test
   ```

5. Build the Switch overlay with Docker/devkitPro, or push and let GitHub Actions build it.
6. Copy the `.ovl` to the SD card and test on hardware.
7. Commit using Conventional Commits, for example:

   ```sh
   git add data/locales/zh-Hans.json source/generated/messages.cpp
   git commit -m "fix(i18n): improve Chinese size labels"
   ```

### C++ project conventions

- Headers under `include/` describe public types and functions.
- Matching `.cpp` files under `source/` contain implementations.
- Code is grouped under the `mhgu` namespace.
- The portable core must not include `switch.h`, `tesla.hpp`, or `dmntcht.h`.
- Raw offsets belong in a `GameProfile`; do not spread numeric addresses through UI or core code.
- Any new memory write must validate its target and read the value back.
- Compile with warnings treated as errors. Fix the warning instead of disabling it.

### Add or edit a language

All locale files must contain exactly the same monster keys and UI keys. After editing:

```sh
python3 tools/generate_catalog.py
make -f Makefile.host test
```

The generator fails with a list of missing or extra keys when a locale is incomplete. Unsupported detected game languages intentionally resolve to English.

### Refresh monster data

The network refresh is separate from localization:

```sh
python3 tools/refresh_catalog.py
python3 tools/validate_crowns.py
python3 tools/generate_catalog.py
make -f Makefile.host test
```

Review the JSON diff before committing. A source website changing its HTML must never silently delete or rename translations.

### Change the version

`VERSION` is the single version source. For a release:

1. Replace its contents, for example `0.1.0` → `0.2.0`.
2. Run the complete tests and Switch build.
3. Commit and tag:

   ```sh
   git add VERSION
   git commit -m "chore(release): v0.2.0"
   git tag v0.2.0
   git push upstream main --tags
   ```

The Makefile passes this value to the Tesla UI and embeds it in the NRO metadata.

### Common problems

- **`DEVKITPRO is not set`**: use the Docker command, or install devkitPro and open a shell where `DEVKITPRO` is exported.
- **Missing `tesla.hpp` or `dmntcht.h`**: run `git submodule update --init --recursive`.
- **Overlay does not appear**: verify the `.ovl` path and that Tesla Menu itself works.
- **`MHGU / MHXX is not running`**: launch the game first and verify the title/version.
- **Scanning never finishes**: enter a quest with a large monster, then choose **Find monster list**.
- **Size does not change**: confirm the preset is not Off and **Apply preset in quest** is enabled. Treat a write error as a safety stop, not something to bypass.
- **A generated file changed unexpectedly**: rerun the generator, review the source JSON diff, and do not hand-edit generated C++.

See [docs/architecture.md](docs/architecture.md) for module boundaries, data flow, memory safety rules, and the future platform-adapter contract.

## Credits

This project is an independent implementation. It does not copy source code, documentation, screenshots, or other assets from the reference projects below.

- [minazuki19/MHGU-Monster-Info-Overlay](https://github.com/minazuki19/MHGU-Monster-Info-Overlay) — Switch prior art and a reference for previously researched MHGU memory behavior.
- [Alexander-Lancellott/MHGU-MHXX-HP-Overlay-For-Switch-Emulator](https://github.com/Alexander-Lancellott/MHGU-MHXX-HP-Overlay-For-Switch-Emulator) — Windows/emulator prior art and a reference for desktop presentation and monster-size investigation.
- [Kiranico MHXX](https://mhxx.kiranico.com/) — monster names, base sizes, and crown thresholds used by the catalog refresh pipeline.
- [MH Crown](https://mhcrown.com/) — independent crown-size and legal quest-size cross-checks.
- [libtesla](https://github.com/minazuki19/libtesla) and [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs) — third-party Switch runtime dependencies, included as Git submodules under their respective licenses.

Monster Hunter and all related names are trademarks of Capcom. This is an unofficial fan project and is not affiliated with or endorsed by Capcom.

## License

[MIT](LICENSE) © 2025 Jing Haihan
