# MHGU Overlay

A multilingual monster HUD and crown-size controller for Monster Hunter
Generations Ultimate on Nintendo Switch.

[![build](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml/badge.svg)](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

> [!IMPORTANT]
> Overlay features modify live game memory. Back up your save before enabling
> memory features, avoid online use, and read
> [Size presets and safety](docs/size-lock.md).
> One-way code patches, including automatic map display, large-monster
> locations, carrying items into the pouch, and battle modifiers remain
> enabled until the game is restarted.

<p align="center">
  <img src="./assets/screenshot.jpg" alt="MHGU monster overlay" width="520" />
</p>

## Features

- Displays each active large monster in a separate lower-left HUD card.
- Shows the localized monster name, current and maximum health, health bar,
  and health percentage in real time.
- Shows the size multiplier, calculated actual size, crown class, and Hyper
  status.
- Switches between the original 30 FPS target and a 60 FPS target.
- Automatically displays the map and marks large-monster locations.
- Allows carried items, such as eggs, to enter the item pouch.
- Adds invincibility plus health, stamina, and sharpness protection under the
  grouped battle-functions menu.
- Unlocks all three Hunter Art slots when enabled.
- Allows repeated Hunter Art use without consuming the gauge.
- Prevents the Valor gauge from decreasing.
- Fills the Alchemy gauge when enabled.
- Keeps the SP status from expiring.
- Enables automatic Bowgun reload.
- Prevents consumable items from decreasing.
- Sets hunter affinity to a user-selected value from 0% to 100%.
- Adds a one-way weapon transmog switch under Equipment transmog.
- Adds a one-way armor transmog switch under Equipment transmog.
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

- **Monster info overlay** hides the settings page and returns input to the game.
- Hold `L3 + R3` in the overlay to return to settings.
- Press `B` on the settings page to close the overlay.
- Choose the language manually or leave it on **Auto**.
- Choose **30 FPS** or **60 FPS**; **30 FPS** is the default.
- Choose a size preset before entering a quest; **Off** is the default.
- **Map & large monster locations** combines automatic map display and large
  monster markers. Its enabled state is not saved, and restarting the game
  restores the original behavior.
- **Carry items into pouch** is also a one-way, non-persistent selector;
  restarting the game restores the original carrying behavior.
- **Battle functions** groups hunter, combat-parameter, and Palico modifiers.
  **Invincible**, **Health does not decrease**, **Stamina does not decrease**,
  and **Sharpness does not decrease** are one-way, non-persistent instruction
  patches.
- **Unlock Hunter Art slots** is also one-way and returns to the original
  behavior only after restarting the game.
- **Unlimited Hunter Arts** applies two one-way instruction patches and also
  requires a game restart to restore.
- **Valor gauge does not decrease** applies three one-way instruction patches.
- **Fill Alchemy gauge** applies two one-way instruction patches.
- **SP status does not expire** is a one-way instruction patch.
- **Bowgun auto reload** is a one-way instruction patch.
- **Consumable items do not decrease** is a one-way instruction patch.
- **Hunter affinity** uses Left/Right for 1% adjustments and L/R for 10%
  adjustments. Press A to enable the selected value. The percentage is saved,
  but the enabled state is not; restarting the game restores the original
  instruction.
- **Weapon transmog** is a one-way instruction patch under **Equipment
  transmog** and remains active until the game is restarted.
- **Armor transmog** is a one-way instruction patch under **Equipment
  transmog** and remains active until the game is restarted.
- Use **Rescan** to discard the cached pointer and scan again.

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
