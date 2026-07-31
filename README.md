# MHGU Overlay

A multilingual monster HUD and crown-size controller for Monster Hunter
Generations Ultimate on Nintendo Switch.

[![build](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml/badge.svg)](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

> [!IMPORTANT]
> Size presets modify live game memory. Back up your save before enabling
> them, avoid online use, and read
> [Size presets and safety](docs/size-lock.md).

<p align="center">
  <img src="./assets/screenshot.jpg" alt="MHGU Overlay compact HUD" width="520" />
</p>

## Features

- Displays each active large monster in a separate lower-left HUD card.
- Shows the localized monster name, current and maximum health, health bar,
  and health percentage in real time.
- Shows the size multiplier, calculated actual size, crown class, and Hyper
  status.
- Optionally locks each monster to its own Mini, Silver, or Gold crown
  threshold; **Off** remains the default.
- English, Simplified Chinese, and Japanese, with automatic detection and
  English fallback.
- Conservative, version-gated memory access with bounds checks and immediate
  write verification.

## Requirements

- MHGU 1.4.0 for Nintendo Switch
- Atmosphère with `dmnt:cht`
- Tesla Menu / ovlmenu

## Install

1. Download `mhgu-overlay.ovl` from the
   [latest release](https://github.com/jinghaihan/mhgu-overlay/releases/latest),
   or download the `mhgu-overlay` artifact from a successful
   [build workflow](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml).
2. Copy it to `sdmc:/switch/.overlays/mhgu-overlay.ovl`.
3. Start MHGU 1.4.0, open Tesla Menu, and select **MHGU Overlay**.

## Use

- **Open compact HUD** hides the settings page and returns input to the game.
- Hold `L3 + R3` in the compact HUD to return to settings.
- Press `B` on the settings page to close the overlay.
- Choose the language manually or leave it on **Auto**.
- Choose a size preset before entering a quest; **Off** is the default.
- Use **Find monster list** to discard the cached pointer and scan again.

Settings are saved to `sdmc:/config/mhgu-overlay/settings.ini`.

## Documentation

- [Development guide](docs/development.md) — setup, testing, building, data,
  localization, and releases
- [Architecture](docs/architecture.md) — module boundaries, runtime flow, and
  future adapters
- [Size presets and safety](docs/size-lock.md) — behavior, validation, and
  hardware-testing limits

## Credits

This is an independent implementation. The following projects and sites were
used as prior art or factual references; their source, documentation,
screenshots, and media are not copied into this repository.

- [MHGU Monster Info Overlay](https://github.com/minazuki19/MHGU-Monster-Info-Overlay)
  — prior research into MHGU memory behavior on Switch.
- [MHGU MHXX HP Overlay for Switch Emulator](https://github.com/Alexander-Lancellott/MHGU-MHXX-HP-Overlay-For-Switch-Emulator)
  — desktop overlay and monster-size prior art.
- [FETH Overlays](https://github.com/3096/feth-overlays) — prior art for
  version-gated Switch memory editing.
- [Kiranico](https://mhxx.kiranico.com/) — authoritative base sizes and
  crown thresholds used by the catalog pipeline.
- [MH Crown](https://mhcrown.com/) — independent crown-size cross-checks.
- [libtesla](https://github.com/minazuki19/libtesla) — Tesla UI runtime
  dependency included as a Git submodule under its own license.
- [Atmosphere Libs](https://github.com/Atmosphere-NX/Atmosphere-libs) — Switch
  process runtime dependency included as a Git submodule under its own license.

Monster Hunter and related names are trademarks of Capcom. This unofficial
fan project is not affiliated with or endorsed by Capcom.

## License

[MIT](./LICENSE) License © [jinghaihan](https://github.com/jinghaihan)
