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

1. Download `mhgu-overlay.ovl` from the
   [latest release](https://github.com/jinghaihan/mhgu-overlay/releases/latest),
   or use the latest successful GitHub Actions artifact before the first
   release is published.
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

## Development

Clone the submodules, run the portable tests, and build the Switch overlay:

```sh
git clone --recurse-submodules https://github.com/jinghaihan/mhgu-overlay.git
cd mhgu-overlay
make -f Makefile.host test
make -j2
```

If you do not have devkitPro, push the branch and download the build artifact
from the latest successful GitHub Actions run. Version tags beginning with `v`
build and publish a GitHub Release automatically.

- [Development guide](docs/development.md) — setup, Docker, data,
  localization, releases, and troubleshooting
- [Architecture](docs/architecture.md) — module boundaries, data flow,
  memory safety, and future platform adapters

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
