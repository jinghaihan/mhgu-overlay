#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = [
#   "questionary==2.1.1",
#   "semver==3.0.4",
# ]
# ///

from __future__ import annotations

import argparse
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional, Sequence

import questionary
from questionary import Choice, Style
from semver import Version


ROOT = Path(__file__).resolve().parent.parent
VERSION_PATH = ROOT / "VERSION"
REMOTE = "origin"

PROMPT_STYLE = Style(
  [
    ("qmark", "fg:#00a67d bold"),
    ("question", "bold"),
    ("answer", "fg:#00a67d bold"),
    ("pointer", "fg:#00a67d bold"),
    ("highlighted", "fg:#00a67d bold"),
    ("selected", "fg:#00a67d"),
  ]
)


class ReleaseError(RuntimeError):
  pass


def command(
  *args: str,
  capture: bool = False,
  check: bool = True,
) -> subprocess.CompletedProcess[str]:
  return subprocess.run(
    args,
    cwd=ROOT,
    check=check,
    capture_output=capture,
    text=True,
  )


def output(*args: str) -> str:
  return command(*args, capture=True).stdout.strip()


def require_command(name: str) -> None:
  if shutil.which(name) is None:
    raise ReleaseError(f"required command not found: {name}")


def ensure_repository_is_ready() -> None:
  for name in ("git", "make"):
    require_command(name)

  try:
    repository = Path(output("git", "rev-parse", "--show-toplevel")).resolve()
  except subprocess.CalledProcessError as error:
    raise ReleaseError("script must run from a Git repository") from error

  if repository != ROOT:
    raise ReleaseError("script must run from the mhgu-overlay repository")
  if output("git", "branch", "--show-current") != "main":
    raise ReleaseError("switch to the main branch before releasing")
  if command("git", "remote", "get-url", REMOTE, capture=True, check=False).returncode:
    raise ReleaseError(f"the {REMOTE} remote is not configured")
  if output("git", "status", "--porcelain"):
    raise ReleaseError("working tree is not clean; commit or stash changes first")

  print(f"Fetching {REMOTE}/main and release tags...", flush=True)
  command(
    "git",
    "fetch",
    REMOTE,
    f"main:refs/remotes/{REMOTE}/main",
    "--tags",
  )
  if output("git", "rev-parse", "HEAD") != output(
    "git", "rev-parse", f"refs/remotes/{REMOTE}/main"
  ):
    raise ReleaseError(f"local main is not synchronized with {REMOTE}/main")


def read_version() -> Version:
  raw_version = VERSION_PATH.read_text(encoding="utf-8").strip()
  try:
    version = Version.parse(raw_version)
  except ValueError as error:
    raise ReleaseError(f"VERSION is not a SemVer value: {raw_version}") from error
  if version.prerelease is not None or version.build is not None:
    raise ReleaseError(f"VERSION must be a stable SemVer value: {raw_version}")
  return version


def next_versions(version: Version) -> dict[str, Version]:
  return {
    "patch": version.bump_patch(),
    "minor": version.bump_minor(),
    "major": version.bump_major(),
  }


def plain_select(version: Version, candidates: dict[str, Version]) -> Optional[str]:
  print(f"\nCurrent version: v{version}\n")
  print(f"  1) patch  v{candidates['patch']}")
  print(f"  2) minor  v{candidates['minor']}")
  print(f"  3) major  v{candidates['major']}")
  print("  q) cancel\n")
  try:
    selection = input("Select release type [1-3/q]: ").strip().lower()
  except EOFError:
    return None
  return {
    "1": "patch",
    "patch": "patch",
    "2": "minor",
    "minor": "minor",
    "3": "major",
    "major": "major",
    "q": None,
    "": None,
  }.get(selection, "invalid")


def select_bump(version: Version, candidates: dict[str, Version]) -> Optional[str]:
  if not sys.stdin.isatty():
    selection = plain_select(version, candidates)
    if selection == "invalid":
      raise ReleaseError("unknown release type")
    return selection

  selection = questionary.select(
    f"Select the next version (current: v{version})",
    choices=[
      Choice(f"patch  v{candidates['patch']}", value="patch"),
      Choice(f"minor  v{candidates['minor']}", value="minor"),
      Choice(f"major  v{candidates['major']}", value="major"),
      Choice("Cancel", value="cancel"),
    ],
    style=PROMPT_STYLE,
    instruction="(use arrow keys)",
  ).ask()
  return None if selection in {None, "cancel"} else selection


def confirm_release(tag: str) -> bool:
  if not sys.stdin.isatty():
    try:
      answer = input("Continue? [y/N]: ").strip().lower()
    except EOFError:
      return False
    return answer in {"y", "yes"}

  return (
    questionary.confirm(
      f"Create and push release {tag}?",
      default=False,
      style=PROMPT_STYLE,
    ).ask()
    is True
  )


def tag_exists(tag: str) -> bool:
  return (
    command(
      "git",
      "rev-parse",
      "--verify",
      "--quiet",
      f"refs/tags/{tag}",
      capture=True,
      check=False,
    ).returncode
    == 0
  )


def rollback_version(version: Version) -> None:
  command("git", "restore", "--staged", "VERSION", capture=True, check=False)
  VERSION_PATH.write_text(f"{version}\n", encoding="utf-8")
  print(f"Restored VERSION to {version}.", file=sys.stderr)


def publish(bump: Optional[str]) -> int:
  ensure_repository_is_ready()
  current_version = read_version()
  candidates = next_versions(current_version)
  selected_bump = bump or select_bump(current_version, candidates)
  if selected_bump is None:
    print("Release cancelled.")
    return 0

  next_version = candidates[selected_bump]
  tag = f"v{next_version}"
  if tag_exists(tag):
    raise ReleaseError(f"tag already exists: {tag}")

  print("\nRelease plan:")
  print(f"  version: v{current_version} -> {tag}")
  print(f"  commit:  chore: release {tag}")
  print(f"  remote:  {REMOTE}/main and {tag}\n")
  if not confirm_release(tag):
    print("Release cancelled.")
    return 0

  print("\nVerifying generated tables...", flush=True)
  command(sys.executable, "tools/generate_catalog.py")
  if output("git", "status", "--porcelain"):
    raise ReleaseError("generated files changed; review and commit them first")

  print("Running host tests...", flush=True)
  command("make", "-f", "Makefile.host", "test")

  version_changed = False
  commit_created = False
  try:
    VERSION_PATH.write_text(f"{next_version}\n", encoding="utf-8")
    version_changed = True
    command("git", "add", "VERSION")
    if output("git", "diff", "--cached", "--name-only") != "VERSION":
      raise ReleaseError("the release commit contains files other than VERSION")

    command("git", "commit", "-m", f"chore: release {tag}")
    commit_created = True
    command("git", "push", REMOTE, "main")
    command("git", "tag", "-a", tag, "-m", tag)
    command("git", "push", REMOTE, tag)
  except BaseException:
    if version_changed and not commit_created:
      rollback_version(current_version)
    raise

  print(f"\nPublished {tag}.")
  print(
    "Release workflow: "
    "https://github.com/jinghaihan/mhgu-overlay/actions/workflows/release.yml"
  )
  return 0


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
  parser = argparse.ArgumentParser(
    description="Create and push an MHGU Overlay SemVer release.",
  )
  parser.add_argument(
    "bump",
    nargs="?",
    choices=("major", "minor", "patch"),
    help="release type; omit to select interactively",
  )
  return parser.parse_args(argv)


def main() -> int:
  args = parse_args()
  try:
    return publish(args.bump)
  except KeyboardInterrupt:
    print("\nRelease cancelled.", file=sys.stderr)
    return 130
  except ReleaseError as error:
    print(f"release failed: {error}", file=sys.stderr)
    return 1
  except subprocess.CalledProcessError as error:
    print(
      f"release failed: command exited with {error.returncode}: "
      f"{shlex.join(error.cmd)}",
      file=sys.stderr,
    )
    if error.stderr:
      print(error.stderr.strip(), file=sys.stderr)
    return error.returncode or 1


if __name__ == "__main__":
  raise SystemExit(main())
