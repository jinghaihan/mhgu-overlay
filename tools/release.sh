#!/bin/sh

set -eu

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd -P)
repo_root=$(CDPATH= cd "$script_dir/.." && pwd -P)

fail() {
  printf 'release failed: %s\n' "$1" >&2
  exit 1
}

usage() {
  cat <<'EOF'
Usage: ./tools/release.sh [major|minor|patch]

Without an argument, the script prompts for the SemVer bump.
It verifies the repository, updates VERSION, runs host tests, creates a
release commit and annotated tag, then pushes main and the tag to upstream.
EOF
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

semver_is_valid() {
  printf '%s\n' "$1" \
    | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'
}

version_changed=0
commit_created=0
current_version=''

cleanup() {
  status=$?
  trap - 0 1 2 15
  if [ "$status" -ne 0 ] \
    && [ "$version_changed" -eq 1 ] \
    && [ "$commit_created" -eq 0 ]; then
    git restore --staged VERSION >/dev/null 2>&1 || true
    printf '%s\n' "$current_version" > VERSION
    printf 'restored VERSION to %s\n' "$current_version" >&2
  fi
  exit "$status"
}

trap cleanup 0
trap 'exit 130' 1 2 15

if [ "$#" -gt 1 ]; then
  usage >&2
  exit 1
fi

case "${1:-}" in
  -h|--help)
    usage
    exit 0
    ;;
  ''|major|minor|patch)
    bump=${1:-}
    ;;
  *)
    usage >&2
    exit 1
    ;;
esac

require_command git
require_command grep
require_command make
require_command python3
require_command tr

cd "$repo_root"

[ "$(git rev-parse --show-toplevel 2>/dev/null)" = "$repo_root" ] \
  || fail 'script must run from the mhgu-overlay repository'
[ "$(git branch --show-current)" = 'main' ] \
  || fail 'switch to the main branch before releasing'
git remote get-url upstream >/dev/null 2>&1 \
  || fail 'the upstream remote is not configured'
[ -z "$(git status --porcelain)" ] \
  || fail 'working tree is not clean; commit or stash changes first'

printf 'Fetching upstream/main and release tags...\n'
git fetch upstream main:refs/remotes/upstream/main --tags

[ "$(git rev-parse HEAD)" = "$(git rev-parse refs/remotes/upstream/main)" ] \
  || fail 'local main is not synchronized with upstream/main'

current_version=$(tr -d '[:space:]' < VERSION)
semver_is_valid "$current_version" \
  || fail "VERSION is not a stable SemVer value: $current_version"

major=${current_version%%.*}
minor_and_patch=${current_version#*.}
minor=${minor_and_patch%%.*}
patch=${minor_and_patch#*.}

next_major="$((major + 1)).0.0"
next_minor="$major.$((minor + 1)).0"
next_patch="$major.$minor.$((patch + 1))"

if [ -z "$bump" ]; then
  printf '\nCurrent version: v%s\n\n' "$current_version"
  printf '  1) patch  v%s\n' "$next_patch"
  printf '  2) minor  v%s\n' "$next_minor"
  printf '  3) major  v%s\n' "$next_major"
  printf '  q) cancel\n\n'
  printf 'Select release type [1-3/q]: '
  read -r choice
  case "$choice" in
    1|patch) bump=patch ;;
    2|minor) bump=minor ;;
    3|major) bump=major ;;
    q|Q|'')
      printf 'Release cancelled.\n'
      exit 0
      ;;
    *) fail "unknown release type: $choice" ;;
  esac
fi

case "$bump" in
  major) next_version=$next_major ;;
  minor) next_version=$next_minor ;;
  patch) next_version=$next_patch ;;
esac

tag="v$next_version"
if git rev-parse --verify --quiet "refs/tags/$tag" >/dev/null; then
  fail "tag already exists: $tag"
fi

printf '\nRelease plan:\n'
printf '  version: v%s -> %s\n' "$current_version" "$tag"
printf '  commit:  chore: release %s\n' "$tag"
printf '  remote:  upstream/main and %s\n\n' "$tag"
printf 'Continue? [y/N]: '
read -r confirmation
case "$confirmation" in
  y|Y|yes|YES) ;;
  *)
    printf 'Release cancelled.\n'
    exit 0
    ;;
esac

printf '\nVerifying generated tables...\n'
python3 tools/generate_catalog.py
[ -z "$(git status --porcelain)" ] \
  || fail 'generated files changed; review and commit them before releasing'

printf 'Running host tests...\n'
make -f Makefile.host test

printf '%s\n' "$next_version" > VERSION
version_changed=1

git add VERSION
[ "$(git diff --cached --name-only)" = 'VERSION' ] \
  || fail 'the release commit contains files other than VERSION'
git commit -m "chore: release $tag"
commit_created=1

git push upstream main
git tag -a "$tag" -m "$tag"
git push upstream "$tag"

version_changed=0
printf '\nPublished %s.\n' "$tag"
printf 'Release workflow: https://github.com/jinghaihan/mhgu-overlay/actions/workflows/release.yml\n'
