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
| **Monster HUD** | Opens the low-profile hunting HUD. Each active large monster gets a lower-left card with its name, health bar, current and maximum health, health percentage, size multiplier, calculated size, crown class, and Hyper status. |
| **Damage display** | Shows animated damage values near the upper center of the hunting HUD when a large monster's health decreases. |
| **Frame rate** | Switches the game's frame-rate target between 30 FPS and 60 FPS. This setting can be changed in either direction. |
| **Size lock** | Selects **Off**, **Mini crown**, **Silver crown**, or **Gold crown**. The chosen per-monster crown threshold is applied when a valid monster object appears; **Off** is the default. |
| **Map & monster icons** | Automatically displays the map and marks large-monster locations. |
| **Carry to pouch** | Allows carried objects such as eggs to be placed in the item pouch. |
| **Equipment transmog** | Opens the weapon and armor appearance submenu. |
| **Battle functions** | Opens the grouped hunter, combat-parameter, resource, and Palico tools. |
| **Address diagnostic** | Opens the research-only quest-data and player-resource address scanners. |
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
| **No health loss** | Prevents hunter health from decreasing. |
| **No stamina loss** | Prevents hunter stamina from decreasing. |
| **No sharpness loss** | Prevents weapon sharpness from decreasing. |
| **Unlock Art slots** | Unlocks all three Hunter Art slots. |
| **Unlimited Arts** | Allows Hunter Arts to be used without consuming their gauge. |
| **No Valor drain** | Prevents the Valor gauge from decreasing. |
| **Full Alchemy gauge** | Keeps the Alchemy gauge filled. |
| **Long Sword gauge** | Sets the gauge from 0% to 100%. |
| **SP level** | Sets SP level from 1 to 4. |
| **No SP expiry** | Prevents SP status from expiring. |
| **Affinity** | Sets hunter affinity from 0% to 100%. |
| **Bowgun auto reload** | Enables automatic Bowgun reload. |

#### Combat parameters

| Menu item | What it does |
| --- | --- |
| **Damage mode** | Selects **Off**, **Instant kill**, or **Leave at 1 HP**. The two active modes are mutually exclusive and can replace one another immediately. |
| **Attack** | Sets the attack multiplier from x1 to x10. |
| **Defense** | Sets the defense multiplier from x1 to x10. |
| **Move speed** | Sets movement speed from x1.0 to x5.0. |

#### Resources

| Menu item | What it does |
| --- | --- |
| **Infinite items** | Prevents consumable-item counts from decreasing. |
| **Zenny** | Sets a selected value from 0 to 9,999,999; defaults to the maximum. |
| **Wycademy pts.** | Sets a selected value from 0 to 9,999,999; defaults to the maximum. |
| **Item pouch slot** | Selects one of the first 10 item-pouch slots. |
| **Quantity** | Selects a quantity from 1 to 99. |
| **Apply quantity** | Performs one bounded, verified byte write to the selected slot. It does not repeat the write automatically. |

#### Palico

| Menu item | What it does |
| --- | --- |
| **No Palico HP loss** | Prevents Palico health from decreasing. |
| **Affinity** | Sets Palico affinity from 0% to 100%. |

## Address diagnostics

Address diagnostics are available in builds from the
[`codex/address-research`](https://github.com/jinghaihan/mhgu-overlay/actions?query=branch%3Acodex%2Faddress-research)
branch. Download the `mhgu-overlay` artifact from its latest successful build.
These are read-only scanners used to locate stable runtime data before a
normal overlay feature is implemented. Open
**Address diagnostic** from the main menu, then choose **Quest data
diagnostic** or **Resource data diagnostic**.

Keep MHGU running throughout each complete test sequence. Closing or
restarting the game changes the Heap base and makes results from different
sessions unsuitable for direct comparison.

### Quest data diagnostic

Disable the fixed-starting-area LayeredFS mod and restart MHGU before this
test. The scanner looks for both serialized quest resources and the live
runtime quest structure; the report labels them `resource` and `runtime`.

1. Do not accept a quest. Open **Quest data diagnostic**, select **Start
   read-only scan**, and wait for **Scan complete**.
2. Accept one quest from the table below. Wait 3–5 seconds without departing,
   then run another scan.
3. Cancel that quest, accept a different quest on another map, wait 3–5
   seconds, and scan again.
4. Send `sdmc:/config/mhgu-overlay/quest-scan.log`. All scans from the current
   overlay session are appended to this file.

You do not need to depart, complete, or abandon a quest in the field. The
recommended first pair is **Coal Hearted** followed by **Kushala Daora
Strikes**. If either is unavailable, use any two unlocked quests from
different maps in this table.

| Quest list | Simplified Chinese title | English title | ID | Map |
| --- | --- | --- | ---: | --- |
| Hub 6★ | 燃石炭，尽管挖吧 | Coal Hearted | 10631 | Volcano |
| Hub 7★ | 冰点下的统治者 | The Frozen Dictator | 10756 | Arctic Ridge (Night) |
| Hub 7★ | 飘然而下的钢龙 | Kushala Daora Strikes | 10757 | Frozen Seaway |
| Hub 7★ | 古代霞龙 | The Elder Dragon of Mist | 10758 | Verdant Hills |
| Hub 7★ | 寂静深处 | Beyond the Silence | 10759 | Marshlands |
| Hub 7★ | 炼狱主人、愤怒的炎帝 | Emperor of Flame | 10760 | Volcano |
| Hub 7★ | 呼啸的灾祸之火 | The Fires of Devastation | 10761 | Volcanic Hollow |
| Hub G4 | 贯穿天际的凶星 | Sky Render | 11401 | Ruined Pinnacle |
| Hub G4 | 暴雪呼唤者 | Blizzard Blower | 11412 | Arctic Ridge (Night) |
| Hub G4 | 灾祸比钢铁更加坚硬 | Steel Yourself | 11413 | Jungle |
| Hub G4 | 出现于虚无之物 | Out of Thin Air | 11414 | Ruined Pinnacle |
| Hub G4 | 消失于虚无之物 | Into Thin Air | 11415 | Verdant Hills |
| Hub G4 | 财宝在爆炎之中 | Explosion Marks the Spot | 11416 | Volcano |
| Hub G4 | 散落在沙漠上的爆炎尘 | Sandblasting | 11417 | Desert |
| Hub G4 | 超级音速弓 | Supersonic | 11458 | Jungle |
| Hub G4 | 天彗龙流 猎人道场 | Hunter Dojo: Valstrax-ryu | 11459 | Desert |
| Hub G4 | 因天彗龙失去理智 | Scales of Justice | 11460 | Arctic Ridge (Night) |

### Resource data diagnostic

This scanner searches for the player structure containing Zenny at `+0x24`
and Wycademy Points at `+0x2C`. Restart MHGU first and do not enable the normal
Zenny or Wycademy Points modifiers during the test.

1. Read the exact current Zenny and Wycademy Points values in the game.
2. In **Resource data diagnostic**, use Left/Right on **Input step** to choose
   `1`, `10`, `100`, and so on. Use Left/Right on either value to change it by
   one step, or `L`/`R` to change it by ten steps.
3. Enter both exact values and select **Start initial scan**. At least one of
   the two values must be nonzero.
4. Buy or sell a low-value item so that Zenny changes. Confirm the new amount
   in the game, enter it in the diagnostic menu, and select **Filter with new
   values**.
5. Use an in-game exchange with a clearly displayed price so that Wycademy
   Points change. Enter the new point total and filter again.
6. Send `sdmc:/config/mhgu-overlay/resource-scan.log`. The initial scan and
   every filter stage are appended to the same report.

Only the initial resource scan traverses the full Heap. Later filter actions
re-read the saved candidates and normally complete much faster. Even if the
first scan returns one candidate, perform both value-change filters so the
address is verified against independent changes.

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
- **Monster HUD** returns input to the game. Hold `L3 + R3` to return
  to settings, and press `B` on a settings page to go back or close the
  overlay.
- Language, Monster HUD, damage display, frame rate, size lock,
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
