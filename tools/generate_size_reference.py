#!/usr/bin/env python3
"""Generate the player-facing MHGU crown size reference."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from decimal import Decimal, ROUND_HALF_UP
import html
import json
from pathlib import Path
import re
import socket
import time
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
CATALOG_PATH = ROOT / "data" / "catalog" / "monsters.json"
LEGAL_RANGES_PATH = ROOT / "data" / "catalog" / "legal-size-ranges.json"
LOCALE_PATH = ROOT / "data" / "locales" / "zh-Hans.json"
OUTPUT_PATH = ROOT / "docs" / "monster-size-reference.md"
USER_AGENT = "mhgu-overlay-catalog/0.1 (+https://github.com/jinghaihan/mhgu-overlay)"
SIZE_PATTERN = re.compile(r"\b(?:\d{1,3}(?:,\d{3})+|\d{3,5})\.\d{2}\b")


@dataclass(frozen=True)
class CrownSize:
  value: Decimal
  special_only: bool


@dataclass(frozen=True)
class MhCrownFacts:
  miniature: tuple[CrownSize, ...]
  gold: tuple[CrownSize, ...]


def formatted_size(base_x100: int, percent: int) -> str:
  value = Decimal(base_x100) * Decimal(percent) / Decimal(10000)
  return str(value.quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))


def size_percent(base_x100: int, size: Decimal) -> str:
  value = size * Decimal(10000) / Decimal(base_x100)
  rounded = value.quantize(Decimal("0.01"), rounding=ROUND_HALF_UP)
  return f"{rounded:.2f}".rstrip("0").rstrip(".") + "%"


def section(document: str, heading: str, *aliases: str) -> str:
  for candidate in (heading, *aliases):
    start_pattern = re.compile(
      rf'<p\s+class="menuLine">\s*{re.escape(candidate)}\s*</p>',
      re.IGNORECASE,
    )
    start = start_pattern.search(document)
    if start is None:
      continue
    remainder = document[start.end() :]
    paragraph = re.search(
      r"<p(?:\s[^>]*)?>(.*?)</p>",
      remainder,
      re.IGNORECASE | re.DOTALL,
    )
    return paragraph.group(1) if paragraph is not None else ""
  return ""


def parse_sizes(fragment: str) -> tuple[CrownSize, ...]:
  parsed = []
  lines = re.sub(r"<br\s*/?>", "\n", fragment, flags=re.IGNORECASE)
  for line in lines.splitlines():
    visible = html.unescape(re.sub(r"<[^>]+>", " ", line))
    normalized = " ".join(visible.split()).lower()
    special_only = any(
      marker in normalized
      for marker in (
        "dlc only",
        "event only",
        "event quest size",
        "イベントクエスト専用サイズ",
        "規格外",
        "特殊個体専用サイズ",
      )
    )
    match = SIZE_PATTERN.search(visible)
    if match is not None:
      parsed.append(CrownSize(Decimal(match.group(0).replace(",", "")), special_only))
  return tuple(parsed)


def fetch_mhcrown(
  url: str,
  timeout: float,
  retries: int,
) -> MhCrownFacts | None:
  last_error: Exception | None = None
  for attempt in range(retries + 1):
    try:
      request = Request(url, headers={"User-Agent": USER_AGENT})
      with urlopen(request, timeout=timeout) as response:
        document = response.read().decode("utf-8")
      facts = MhCrownFacts(
        parse_sizes(section(document, "Miniature Crown", "最小金冠")),
        parse_sizes(section(document, "Gold Crown", "最大金冠")),
      )
      if not facts.miniature and not facts.gold:
        return None
      if not facts.miniature or not facts.gold:
        missing = "Miniature Crown" if not facts.miniature else "Gold Crown"
        raise RuntimeError(f"{url}: {missing} section contains no sizes")
      return facts
    except (HTTPError, URLError, OSError, socket.timeout) as error:
      last_error = error
      if attempt < retries:
        time.sleep(0.5 * (attempt + 1))
  raise RuntimeError(f"{url}: {last_error}")


def extreme(sizes: tuple[CrownSize, ...], smallest: bool) -> CrownSize | None:
  normal = [size for size in sizes if not size.special_only]
  values = normal or list(sizes)
  if not values:
    return None
  return (min if smallest else max)(values, key=lambda size: size.value)


def special_extreme(
  sizes: tuple[CrownSize, ...],
  smallest: bool,
) -> CrownSize | None:
  special = [size for size in sizes if size.special_only]
  if not special:
    return None
  return (min if smallest else max)(special, key=lambda size: size.value)


def size_text(label: str, size: CrownSize | None, base_x100: int) -> str:
  if size is None:
    return f"{label}—"
  return f"{label}{size.value:.2f} ({size_percent(base_x100, size.value)})"


def comparison_text(monster: dict, facts: MhCrownFacts | None) -> str:
  if facts is None:
    return "MH Crown unavailable"
  listed = {size.value for size in facts.miniature + facts.gold}
  expected = {
    Decimal(formatted_size(monster["baseSizeX100"], monster["crowns"]["miniPercent"])),
    Decimal(formatted_size(monster["baseSizeX100"], monster["crowns"]["goldPercent"])),
  }
  if expected <= listed:
    return "Thresholds match"
  is_approximate = all(
    any(abs(value - actual) <= Decimal("0.01") for actual in listed)
    for value in expected
  )
  if is_approximate:
    return "Thresholds match within 0.01"
  return "⚠ Threshold mismatch"


def mhcrown_text(monster: dict, facts: MhCrownFacts | None) -> str:
  if facts is None:
    return f"—<br>[Source]({monster['sources']['crownValidation']})"
  base_x100 = monster["baseSizeX100"]
  parts = [
    size_text("Smallest miniature: ", extreme(facts.miniature, True), base_x100),
    size_text("Largest gold: ", extreme(facts.gold, False), base_x100),
  ]
  special_mini = special_extreme(facts.miniature, True)
  special_gold = special_extreme(facts.gold, False)
  if special_mini is not None:
    parts.append(size_text("Special minimum: ", special_mini, base_x100))
  if special_gold is not None:
    parts.append(size_text("Special maximum: ", special_gold, base_x100))
  parts.append(f"[Source]({monster['sources']['crownValidation']})")
  return "<br>".join(parts)


def kiranico_text(monster: dict) -> str:
  base_x100 = monster["baseSizeX100"]
  mini = monster["crowns"]["miniPercent"]
  gold = monster["crowns"]["goldPercent"]
  return (
    f"Miniature ≤ {formatted_size(base_x100, mini)} ({mini}%)<br>"
    f"Gold ≥ {formatted_size(base_x100, gold)} ({gold}%)"
  )


def generate_document(
  monsters: list[dict],
  names: dict[str, str],
  facts: dict[str, MhCrownFacts],
) -> str:
  rows = []
  mismatch_count = 0
  approximate_count = 0
  unavailable_count = 0
  for monster in monsters:
    mhcrown = facts.get(monster["key"])
    comparison = comparison_text(monster, mhcrown)
    mismatch_count += comparison.startswith("⚠")
    approximate_count += comparison.startswith("Thresholds match within")
    unavailable_count += mhcrown is None
    name = names[monster["key"]]
    source = monster["sources"]["sizeAndNames"]
    rows.append(
      f"| [{name}]({source}) | {kiranico_text(monster)} | "
      f"{mhcrown_text(monster, mhcrown)} | {comparison} |"
    )
  if mismatch_count == 0:
    mismatch_summary = "none fail to match"
  elif mismatch_count == 1:
    mismatch_summary = "1 does not match"
  else:
    mismatch_summary = f"{mismatch_count} do not match"

  return "\n".join(
    [
      "# MHGU Crown Size Extremes",
      "",
      "> [!NOTE]",
      "> This page is a hunting-record reference. It does not affect the Overlay's",
      "> size-writing limits, which remain the conservative Kiranico crown thresholds.",
      "",
      "The Kiranico column contains **crown thresholds**, not absolute quest extrema.",
      "The MH Crown column shows the smallest miniature and largest gold-crown sizes",
      "listed on each monster page. A special value is marked by MH Crown as DLC-only,",
      "event-only, exclusive to a special individual, or otherwise outside the regular",
      "range. Percentages are calculated from Kiranico's base size; display differences",
      "may be caused by rounding at the source.",
      "",
      f"The table covers {len(monsters)} monsters with variable sizes. "
      f"{approximate_count} threshold comparisons differ only by 0.01, "
      f"{mismatch_summary}, and "
      f"{unavailable_count} MH Crown pages "
      "do not provide crown-size data. Fixed-size monsters and monsters without crown "
      "classification are omitted.",
      "",
      "| Monster | Kiranico thresholds | MH Crown extremes | Comparison |",
      "|---|---:|---:|---|",
      *rows,
      "",
      "## How to read the table",
      "",
      "- `Smallest miniature` and `Largest gold` exclude values carrying an explicit",
      "  special marker on the MH Crown page.",
      "- `Special minimum/maximum` identifies DLC, event, special-individual, or",
      "  out-of-range sizes. It is useful for record checking but is not a regular",
      "  quest extremum.",
      "- Some ordinary quests deliberately exceed the usual gold-crown range. These",
      "  legitimate quest-specific outliers remain in the listed extremes.",
      "- `Thresholds match` means that MH Crown lists both Kiranico crown boundaries;",
      "  it does not mean that the two sites claim identical absolute extrema.",
      "- A difference of 0.01 is treated as source display rounding.",
      "- `Threshold mismatch` requires manual review of both linked pages and must not",
      "  be used to widen the Overlay's size-writing range.",
      "- `—` means the source page did not provide usable crown-size data when this",
      "  reference was generated. It does not mean that the size cannot exist in-game.",
      "",
      "## Data sources",
      "",
      "- [Kiranico MHXX](https://mhxx.kiranico.com/en/mon): base sizes and crown",
      "  thresholds.",
      "- [MH Crown MHXX/MHGU](https://mhcrown.com/): discrete crown sizes and",
      "  special-size markers.",
      "",
      "This document summarizes numerical facts only. It does not reproduce source",
      "quest tables, prose, or media.",
      "",
    ]
  )


def main() -> None:
  parser = argparse.ArgumentParser()
  parser.add_argument("--timeout", type=float, default=20.0)
  parser.add_argument("--retries", type=int, default=2)
  parser.add_argument("--jobs", type=int, default=4)
  parser.add_argument("--output", type=Path, default=OUTPUT_PATH)
  args = parser.parse_args()

  catalog = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
  legal_ranges = json.loads(LEGAL_RANGES_PATH.read_text(encoding="utf-8"))
  locale = json.loads(LOCALE_PATH.read_text(encoding="utf-8"))
  monsters = [monster for monster in catalog["monsters"] if monster["variableSize"]]
  expected_unavailable = {
    key
    for key, value in legal_ranges["monsters"].items()
    if value["validation"] == "mhcrown-unavailable"
  }
  facts: dict[str, MhCrownFacts] = {}
  failures = []
  with ThreadPoolExecutor(max_workers=max(1, min(args.jobs, 8))) as executor:
    futures = {
      executor.submit(
        fetch_mhcrown,
        monster["sources"]["crownValidation"],
        max(1.0, args.timeout),
        max(0, min(args.retries, 5)),
      ): monster
      for monster in monsters
    }
    for completed, future in enumerate(as_completed(futures), start=1):
      monster = futures[future]
      try:
        fetched = future.result()
        if fetched is None:
          if monster["key"] not in expected_unavailable:
            failures.append(f"{monster['key']}: MH Crown page contains no crown sizes")
          print(f"unavailable {completed:02}/{len(monsters)} {monster['key']}")
        else:
          facts[monster["key"]] = fetched
          print(f"fetched {completed:02}/{len(monsters)} {monster['key']}")
      except RuntimeError as error:
        failures.append(str(error))
        print(f"failed {completed:02}/{len(monsters)} {monster['key']}")

  if failures:
    raise RuntimeError(
      "Could not generate a complete reference:\n" + "\n".join(failures)
    )
  output = args.output.resolve()
  output.write_text(
    generate_document(monsters, locale["monsters"], facts),
    encoding="utf-8",
  )
  print(f"Wrote {output}")


if __name__ == "__main__":
  main()
