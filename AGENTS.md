# Repository instructions

## Project scope

- The current deliverable is the Nintendo Switch Tesla overlay for MHGU.
- Keep the core portable, but do not implement Windows or Ryujinx adapters
  unless they are explicitly requested.
- Do not describe the overlay as hardware-verified until the relevant behavior
  has been tested on a real Switch.

## Independent implementation

- The projects credited in `README.md` are prior art, not source templates.
- Do not copy their source code, documentation, screenshots, or other assets.
- Reverse-engineered facts such as offsets and observed data layouts may be
  independently reimplemented and must remain isolated in platform profiles.
- Inspect reference repositories by their external paths. Do not add them as
  remotes to this repository or import their tags and branch history.

## Architecture

- Keep platform-independent decisions in `include/mhgu/core/` and
  `source/core/`.
- The core must not include libnx, Tesla, `dmnt:cht`, filesystem paths, or raw
  game offsets.
- Keep Switch process access, language detection, memory layouts, and offsets
  in `include/mhgu/platform/switch/` and `source/platform/switch/`.
- Keep Tesla rendering and input in `source/ui/`.
- Any new memory write must validate the target identity, enforce bounds, and
  verify the result with an immediate read-back.

## Data and localization

- Store gameplay facts under `data/catalog/`.
- Store every user-facing translation under `data/locales/<locale>.json`.
- English, Simplified Chinese, and Japanese are the supported locales.
  Unsupported languages and detection failures fall back to English.
- Locale files must have identical monster and UI keys.
- Do not edit files under `source/generated/` by hand. After changing catalog
  or locale data, run:

  ```sh
  python3 tools/generate_catalog.py
  make -f Makefile.host test
  ```

- Treat Kiranico and MH Crown as factual data sources. Do not mirror their page
  prose, quest tables, or media.
- Treat Kiranico as authoritative for base sizes and crown thresholds. Use MH
  Crown only as a cross-check and never widen a Kiranico-derived write range
  because of a conflicting page.
- Legal write ranges are global per monster, not per map or quest. Enforce
  them in both the portable core and the final platform write adapter.

## Verification

- Run `make -f Makefile.host test` after changes to core logic, data,
  localization, settings, or memory decoding.
- Build the full `.ovl` with `make -j2` in a devkitPro environment, or verify
  both GitHub Actions jobs before considering a Switch build successful.
- A green CI build proves compilation and host-test success only. Track real
  hardware behavior separately.

## Formatting

- Use two spaces for indentation in C++, Python, JSON, YAML, and other text
  files.
- Preserve the required tab indentation for Makefile recipes.
- Follow `.editorconfig` and format C++ files with the repository's
  `.clang-format` rules.

## Git and releases

- Use Conventional Commits, for example:
  - `feat(core): add quest lifecycle state`
  - `fix(switch): validate the game build id`
  - `docs: clarify hardware verification`
  - `ci: publish tagged releases`
- Split distinct concerns into separate commits and push completed commits to
  `origin`.
- Preserve unrelated user changes and never rewrite shared history unless the
  user explicitly requests it.
- `VERSION` is the single release version source. Release tags must equal
  `v<VERSION>`.
- Use `uv run tools/release.py` for normal releases. It creates the version commit,
  annotated tag, and explicit pushes after running the host checks.
- Push release tags explicitly, for example
  `git push origin v0.1.0`. Never use `git push --tags`.
- Tags matching `v*` trigger `.github/workflows/release.yml`, which builds the
  overlay, generates notes with changelogithub, and uploads the `.ovl` plus
  its checksum.

## Documentation

- Keep `README.md` concise and user-oriented.
- Put contributor setup and build details in `docs/development.md`.
- Put module boundaries and runtime flows in `docs/architecture.md`.
- Keep Credits for all three reference projects, Kiranico, MH Crown, libtesla,
  and Atmosphere-libs.
