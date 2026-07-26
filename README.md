# MHGU Overlay

A multilingual Monster Hunter Generations Ultimate overlay for Nintendo Switch, with live health, crown-aware size data, and experimental quest size presets.

[![build](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml/badge.svg)](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Features

- Compact in-game HUD for monster health, health percentage, size multiplier, actual size, crown class, and Hyper status.
- English, Simplified Chinese, and Japanese monster names and UI.
- Automatic language detection from the game's application control data. Unsupported languages and detection failures fall back to English.
- Use one size-lock selector for Off, Mini, Silver, or Gold. The selected value is resolved separately for each monster and verified after writing.
- Portable C++ core with platform-specific memory, language, persistence, and UI adapters.
- Normalized data for 94 large monsters, generated from independent catalog and locale files.

The size-writing feature is experimental and disabled by default. Read [Size presets](#size-presets) before enabling it.

## Compatibility

| Target | Status |
| --- | --- |
| MHGU 1.4.0 on Atmosphère | Primary target; Title ID and build ID are checked, hardware verification is still required |
| MHXX Nintendo Switch | Not currently supported; it requires a separately verified profile |
| Ryujinx | This Tesla build does not run in Ryujinx; the portable core is designed for a future emulator adapter |
| Windows | Not included; a future adapter can reuse the core without depending on Tesla or libnx |

This repository is under active development. A successful CI build proves that the overlay compiles, not that every memory operation has been validated on every firmware, region, or game build.

## Requirements

- A Nintendo Switch running a current Atmosphère release.
- Tesla Menu / ovlmenu.
- Atmosphère's `dmnt:cht` service.
- MHGU 1.4.0 for Nintendo Switch.

## Install

1. Download `mhgu-overlay.ovl` from the
   [latest release](https://github.com/jinghaihan/mhgu-overlay/releases/latest),
   or use the latest successful GitHub Actions artifact before the first
   release is published.
2. Copy it to:

   ```text
   sdmc:/switch/.overlays/mhgu-overlay.ovl
   ```

3. Start MHGU 1.4.0.
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
- **Size lock**: cycles through Off, Mini (small gold), Silver (large
  silver), and Gold (large gold).
- **Find monster list**: discards the cached in-memory pointer and scans again.

In compact HUD mode, hold the left and right sticks together to return to the settings screen.

## Size presets

Choose **Size lock** before entering the quest. The overlay does not edit a
quest definition or save file in advance. It waits for each large monster's
runtime object, verifies its identity, writes the selected crown threshold,
and reads it back. Choose **Off** to disable all size writes.

Safety rules in the current implementation:

- Off is the default.
- No free-form multiplier can be entered.
- Fixed-size monsters do not receive write requests.
- Each target comes from that monster's Kiranico Mini, Silver, or Gold
  threshold and must remain inside its independent conservative write range.
- Both the portable core and the Switch adapter reject a target outside that
  monster's range.
- The adapter verifies the game build, current-map state, and monster identity
  immediately before writing.
- Only the size multiplier field is written.
- A successful write must pass an immediate read-back check.

The range is global per monster; it is not split by map or quest. This means a
selected crown threshold may be applied in a quest that would not normally
roll that crown. Back up your save before testing. Crown registration, model
scale, hitboxes, quest completion records, and special-event monsters still
need systematic hardware validation. Do not assume this feature is safe for
online or competitive use.

## Languages and data

Localization is intentionally separate from gameplay data:

```text
data/
├── catalog/          # IDs, crown facts, legal write ranges, adapter aliases
├── locales/          # en, zh-Hans, ja
└── schema/           # validation contracts
```

To refresh public numeric facts and regenerate C++ tables:

```sh
python3 tools/refresh_catalog.py
python3 tools/refresh_legal_sizes.py
python3 tools/generate_catalog.py
make -f Makefile.host test
```

Kiranico is authoritative for base sizes and crown thresholds. The legal-range
refresh uses MH Crown only as a recorded cross-check; a disagreement never
widens Kiranico's conservative per-monster range. These tools import factual
values only and do not mirror page prose, quest tables, or media. Locale files
remain authoritative for all user-facing names and messages.

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
- [3096/feth-overlays](https://github.com/3096/feth-overlays) — reference for version-gated Switch memory-editing interaction and validation patterns.
- [Kiranico MHXX](https://mhxx.kiranico.com/) — monster names, base sizes, and crown thresholds used by the catalog refresh pipeline.
- [MH Crown](https://mhcrown.com/) — independent crown-size cross-checks.
- [libtesla](https://github.com/minazuki19/libtesla) and [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs) — third-party Switch runtime dependencies, included as Git submodules under their respective licenses.

Monster Hunter and all related names are trademarks of Capcom. This is an unofficial fan project and is not affiliated with or endorsed by Capcom.

## License

[MIT](LICENSE) © 2025 Jing Haihan
