# MHGU Overlay

An in-game hunting companion for Monster Hunter Generations Ultimate 1.4.0 on
Nintendo Switch. It combines a low-profile large-monster HUD and crown-size
controller with version-gated gameplay tools for frame rate, equipment
appearance, hunter and Palico combat values, resources, and item-pouch edits.

[![build](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml/badge.svg)](https://github.com/jinghaihan/mhgu-overlay/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

> [!IMPORTANT]
> Overlay features modify live game memory. Back up your save before enabling
> memory features, avoid online use, and read
> [Size presets and safety](docs/size-lock.md).
> One-way code patches cannot be disabled from the menu after they have been
> applied. Exit the overlay and restart the game to restore the original game
> code.

<p align="center">
  <img src="./assets/screenshot.jpg" alt="MHGU monster overlay" width="520" />
</p>

## Menu and features

### Main menu

The main menu is ordered as follows:

| Menu item | What it does |
| --- | --- |
| **Language** | Uses automatic game-language detection or selects English, Simplified Chinese, or Japanese manually. |
| **Status** | Shows whether MHGU 1.4.0 is detected, scanning, ready, unsupported, or unable to read or write memory. |
| **Monster info overlay** | Opens the low-profile hunting HUD. Each active large monster gets a lower-left card with its name, health bar, current and maximum health, health percentage, size multiplier, calculated size, crown class, and Hyper status. |
| **Damage display** | Shows animated damage values near the upper center of the hunting HUD when a large monster's health decreases. |
| **Frame rate** | Switches the game's frame-rate target between 30 FPS and 60 FPS. This setting can be changed in either direction. |
| **Size lock** | Selects **Off**, **Mini crown**, **Silver crown**, or **Gold crown**. The chosen per-monster crown threshold is applied when a valid monster object appears; **Off** is the default. |
| **Map & large monster locations** | Automatically displays the map and marks large-monster locations. |
| **Carry items into pouch** | Allows carried objects such as eggs to be placed in the item pouch. |
| **Equipment transmog** | Opens the weapon and armor appearance submenu. |
| **Battle functions** | Opens the grouped hunter, combat-parameter, resource, and Palico tools. |
| **Rescan** | Discards the cached monster-list pointer and scans for it again. |

### Equipment transmog

| Menu item | What it does |
| --- | --- |
| **Weapon transmog** | Enables the game's weapon appearance/transmog path. |
| **Armor transmog** | Enables the game's armor appearance/transmog path. |

Both transmog options are one-way code patches for the current game process.

### Battle functions

#### Hunter

| Menu item | What it does |
| --- | --- |
| **Invincible** | Enables the hunter invincibility patch. |
| **Health does not decrease** | Prevents hunter health from decreasing. |
| **Stamina does not decrease** | Prevents hunter stamina from decreasing. |
| **Sharpness does not decrease** | Prevents weapon sharpness from decreasing. |
| **Unlock Hunter Art slots** | Unlocks all three Hunter Art slots. |
| **Unlimited Hunter Arts** | Allows Hunter Arts to be used without consuming their gauge. |
| **Valor gauge does not decrease** | Prevents the Valor gauge from decreasing. |
| **Fill Alchemy gauge** | Keeps the Alchemy gauge filled. |
| **Long Sword Spirit Gauge** | Sets the gauge from 0% to 100%. |
| **SP level** | Sets SP level from 1 to 4. |
| **SP status does not expire** | Prevents SP status from expiring. |
| **Hunter affinity** | Sets hunter affinity from 0% to 100%. |
| **Bowgun auto reload** | Enables automatic Bowgun reload. |
| **Consumable items do not decrease** | Prevents consumable-item counts from decreasing. |

#### Combat parameters

| Menu item | What it does |
| --- | --- |
| **Monster damage mode** | Selects **Off**, **Instant kill**, or **Leave monster at 1 HP**. The two active modes are mutually exclusive and can replace one another immediately. |
| **Attack multiplier** | Sets the attack multiplier from x1 to x10. |
| **Defense multiplier** | Sets the defense multiplier from x1 to x10. |
| **Movement speed multiplier** | Sets movement speed from x1.0 to x5.0. |

#### Resources

| Menu item | What it does |
| --- | --- |
| **Zenny** | Sets a selected value from 0 to 9,999,999. |
| **Wycademy Points** | Sets a selected value from 0 to 9,999,999. |
| **Item pouch slot** | Selects one of the first 10 item-pouch slots. |
| **Quantity** | Selects a quantity from 1 to 99. |
| **Apply item quantity** | Performs one bounded, verified byte write to the selected slot. It does not repeat the write automatically. |

#### Palico

| Menu item | What it does |
| --- | --- |
| **Palico health does not decrease** | Prevents Palico health from decreasing. |
| **Palico affinity** | Sets Palico affinity from 0% to 100%. |

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

## Controls and persistence

- Press `A` to open a submenu, cycle a selector, enable a patch, or apply the
  selected value.
- Use Left/Right for small numeric adjustments and `L`/`R` for larger steps.
  Affinity and gauge percentages use steps of 1/10; movement speed uses
  0.1/0.5; Zenny and Wycademy Points use 10,000/1,000,000; item-pouch slot and
  quantity use 1/5 and 1/10 respectively. SP level, attack, and defense change
  by one with either control.
- **Monster info overlay** returns input to the game. Hold `L3 + R3` to return
  to settings, and press `B` on a settings page to go back or close the
  overlay.
- Language, monster info overlay, damage display, frame rate, size lock,
  selected numeric values, and the two item-pouch inputs are saved to
  `sdmc:/config/mhgu-overlay/settings.ini`.
- Runtime patch enabled states and monster damage mode are not saved. Numeric
  rows save the selected value, but pressing `A` is required to enable that
  value again after reloading the overlay.
- Selecting **Off** for monster damage mode stops further writes. If an active
  damage mode or another one-way patch has already changed game code, exit the
  overlay and restart the game to restore the original instruction.

## Documentation

- [Development guide](docs/development.md) — setup, testing, building, data,
  localization, and releases
- [Architecture](docs/architecture.md) — module boundaries, runtime flow, and
  future adapters
- [Size presets and safety](docs/size-lock.md) — behavior and write-safety
  limits

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
- [旧大陆的革新之风：3G/4G/GU现代化工具下载&教程](https://www.bilibili.com/video/BV1A8Gw6yEHg/)
  by [hua莱士](https://space.bilibili.com/16486100) — prior art for the
  damage-display behavior.
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
