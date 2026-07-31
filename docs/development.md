# Development guide

This guide is written for contributors who are new to C++ or Nintendo Switch
overlay development. You do not need to understand the whole project before
making a small change.

For the design and platform boundaries, read
[Architecture](architecture.md).

## Get the source

This project uses two Git submodules. Clone it with:

```sh
git clone --recurse-submodules https://github.com/jinghaihan/mhgu-overlay.git
cd mhgu-overlay
```

If you already cloned it without the submodules:

```sh
git submodule update --init --recursive
```

## Where to make changes

| Goal | Edit |
| --- | --- |
| Change English UI text or monster names | `data/locales/en.json` |
| Change Simplified Chinese text or names | `data/locales/zh-Hans.json` |
| Change Japanese UI text or names | `data/locales/ja.json` |
| Change monster size or crown data | `data/catalog/monsters.seed.json`, `data/catalog/legal-size-ranges.json`, and the refresh pipeline |
| Change health, size, or preset decisions | `include/mhgu/core/` and `source/core/` |
| Change Switch addresses or title support | `source/platform/switch/game_profile.cpp` |
| Change memory validation, reads, or writes | `source/platform/switch/monster_reader.cpp` |
| Change the settings menu or compact HUD | `source/ui/main.cpp` |
| Change saved settings | `source/app/settings.cpp` |

Files under `source/generated/` are generated output. Do not edit them by
hand.

## Format source

Use the repository's two-space rules before testing:

```sh
git ls-files '*.cpp' '*.hpp' \
  | grep -v '^source/generated/' \
  | xargs clang-format -i
uvx ruff@0.12.7 format tools
uvx ruff@0.12.7 check tools
```

The Python commands use an isolated Ruff executable managed by
[uv](https://docs.astral.sh/uv/), so Ruff does not become a runtime dependency
of the overlay. Regenerate `source/generated/` through the catalog generator
instead of formatting those files directly.

## Run host tests

The portable core and adapters have ordinary desktop tests. On macOS or Linux
with a C++17 compiler:

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

Run these tests after changing core logic, data, translations, settings, or
memory decoding. `Makefile.host` does not build the Switch plugin.

## Build the Switch overlay

### GitHub Actions

This path does not require a local C++ or Switch toolchain:

1. Push your branch to GitHub, or open **Actions → build → Run workflow** to
   start a build manually without publishing a release.
2. Open the resulting `build` workflow run.
3. Wait for both `host-tests` and `switch` to turn green.
4. Download the `mhgu-overlay` artifact from the workflow summary. It contains
   `mhgu-overlay.ovl` and is retained for 14 days.

Every push and pull request runs the same checks. Only a `v*` tag invokes the
separate release workflow; an ordinary or manual build never creates a GitHub
Release.

### Docker

Docker is the simplest reproducible local build. The image includes devkitA64
and libnx:

```sh
docker run --rm \
  -v "$PWD:/project" \
  -w /project \
  devkitpro/devkita64:latest \
  make -j2
```

The result is `mhgu-overlay.ovl` in the repository root. Docker Desktop,
OrbStack, or another Docker-compatible runtime must be running.

### Local devkitPro

1. Install devkitPro and the `switch-dev` toolchain using the
   [official devkitPro setup guide](https://devkitpro.org/wiki/Getting_Started).
2. Confirm that the environment is available:

   ```sh
   echo "$DEVKITPRO"
   ```

3. Build:

   ```sh
   make -j2
   ```

4. For a clean rebuild:

   ```sh
   make clean
   make -j2
   ```

## Normal development loop

1. Create a branch:

   ```sh
   git switch -c feat/my-change
   ```

2. Edit the relevant source or data file.
3. If you changed a locale or catalog file, regenerate the C++ tables:

   ```sh
   python3 tools/generate_catalog.py
   ```

4. Run the host tests:

   ```sh
   make -f Makefile.host test
   ```

5. Build with Docker or devkitPro, or push and let GitHub Actions build it.
6. Copy the `.ovl` to the SD card and test it on hardware.
7. Commit using Conventional Commits:

   ```sh
   git add data/locales/zh-Hans.json source/generated/messages.cpp
   git commit -m "fix(i18n): improve Chinese size labels"
   ```

## C++ project conventions

- Use two spaces for indentation. The repository includes `.editorconfig` for
  supported editors, `.clang-format` for C++ formatting, and `ruff.toml` for
  Python formatting. Makefile recipes remain tab-indented because `make`
  requires it.
- Headers under `include/` describe public types and functions.
- Matching `.cpp` files under `source/` contain implementations.
- Code is grouped under the `mhgu` namespace.
- The portable core must not include `switch.h`, `tesla.hpp`, or
  `dmntcht.h`.
- Raw offsets belong in a `GameProfile`; do not spread numeric addresses
  through UI or core code.
- Any new memory write must validate its target and read the value back.
- Warnings are treated as errors for project code.

## Localization

All locale files must contain exactly the same monster and UI keys. After
editing a locale:

```sh
python3 tools/generate_catalog.py
make -f Makefile.host test
```

The generator reports missing or extra keys when a locale is incomplete.
Unsupported detected game languages intentionally resolve to English.

## Monster data

Localization and language-independent monster facts are maintained
separately. To refresh public facts, cross-check crown sizes, regenerate C++
tables, and test:

```sh
python3 tools/refresh_catalog.py
python3 tools/refresh_legal_sizes.py
python3 tools/generate_catalog.py
make -f Makefile.host test
```

Review the JSON diff before committing. The refresh process must not silently
delete or rename translations when a source website changes its HTML.
Kiranico is authoritative for base sizes and crown thresholds.
`refresh_legal_sizes.py` records MH Crown comparison results without widening
the Kiranico-derived write range. The range is per monster, not per quest or
map.

## Release version

`VERSION` is the single version source. For a release:

1. Replace its contents with the intended `X.Y.Z` version.
2. Run the complete tests and Switch build.
3. Replace `X.Y.Z` below with that same version, then commit and tag:

   ```sh
   git add VERSION
   git commit -m "chore(release): prepare vX.Y.Z"
   git tag vX.Y.Z
   git push upstream main
   git push upstream vX.Y.Z
   ```

Pushing a tag beginning with `v` starts `.github/workflows/release.yml`. The
workflow rejects a tag that does not equal `v` plus the value in `VERSION`,
builds the overlay, uses
[changelogithub](https://github.com/antfu/changelogithub) to generate the
release notes from Conventional Commits, and uploads both
`mhgu-overlay.ovl` and its SHA-256 checksum.

The Makefile also passes the `VERSION` value to the Tesla UI and embeds it in
the NRO metadata.

## Troubleshooting

- **`DEVKITPRO is not set`**: use Docker, or install devkitPro and open a
  shell where `DEVKITPRO` is exported.
- **Missing `tesla.hpp` or `dmntcht.h`**: run
  `git submodule update --init --recursive`.
- **Overlay does not appear**: verify the `.ovl` path and confirm that Tesla
  Menu itself works.
- **`MHGU is not running`**: launch MHGU 1.4.0 and verify the title and
  version.
- **Scanning never finishes**: enter a quest with a large monster, then
  choose **Find monster list**.
- **Size does not change**: confirm that **Size lock** is not Off. Treat a
  write error as a safety stop.
- **A generated file changed unexpectedly**: rerun the generator, review the
  source JSON diff, and do not hand-edit generated C++.
