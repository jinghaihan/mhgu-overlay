#!/usr/bin/env python3
"""Refresh per-monster legal size ranges from MH Crown."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from decimal import Decimal, ROUND_HALF_UP
import json
from pathlib import Path
import re
import time
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
CATALOG_PATH = ROOT / "data" / "catalog" / "monsters.json"
OUTPUT_PATH = ROOT / "data" / "catalog" / "legal-size-ranges.json"
USER_AGENT = "mhgu-overlay-catalog/0.1 (+https://github.com/jinghaihan/mhgu-overlay)"


def page_sizes(url: str, timeout: float) -> set[Decimal]:
  request = Request(url, headers={"User-Agent": USER_AGENT})
  with urlopen(request, timeout=timeout) as response:
    document = response.read().decode("utf-8")
  return {
    Decimal(value.replace(",", ""))
    for value in re.findall(r"\b\d{3,5}(?:,\d{3})*\.\d{2}\b", document)
  }


def legal_percentages(base_size_x100: int, sizes: set[Decimal]) -> set[int]:
  base_size = Decimal(base_size_x100) / Decimal(100)
  if base_size <= 0:
    return set()
  return {
    int(((size * Decimal(100)) / base_size).to_integral_value(rounding=ROUND_HALF_UP))
    for size in sizes
    if Decimal(50) <= (size * Decimal(100)) / base_size <= Decimal(200)
  }


def fetch_range(
  monster: dict,
  timeout: float,
  retries: int,
) -> tuple[str, dict]:
  error: Exception | None = None
  for attempt in range(retries + 1):
    try:
      percentages = legal_percentages(
        monster["baseSizeX100"],
        page_sizes(monster["sources"]["crownValidation"], timeout),
      )
      mini = monster["crowns"]["miniPercent"]
      gold = monster["crowns"]["goldPercent"]
      if not percentages:
        validation = "mhcrown-unavailable"
      elif mini in percentages and gold in percentages:
        validation = "mhcrown-matched"
      else:
        validation = "mhcrown-mismatch"
      return monster["key"], {
        "minPercent": mini,
        "maxPercent": gold,
        "validation": validation,
      }
    except (HTTPError, URLError, OSError, RuntimeError) as caught:
      error = caught
      if attempt < retries:
        time.sleep(0.5 * (attempt + 1))
  raise RuntimeError(f"{monster['key']}: {error}")


def build_ranges(timeout: float, retries: int, jobs: int) -> dict:
  catalog = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
  monsters = [monster for monster in catalog["monsters"] if monster["variableSize"]]
  ranges = {}
  failures = []
  with ThreadPoolExecutor(max_workers=jobs) as executor:
    futures = [
      executor.submit(fetch_range, monster, timeout, retries) for monster in monsters
    ]
    for completed, future in enumerate(as_completed(futures), start=1):
      try:
        key, size_range = future.result()
        ranges[key] = size_range
        print(f"fetched {completed:02}/{len(monsters)} {key}")
      except RuntimeError as error:
        failures.append(str(error))

  if failures:
    raise RuntimeError(
      "Could not refresh every legal size range:\n" + "\n".join(sorted(failures))
    )

  validation_counts = {
    state: sum(size_range["validation"] == state for size_range in ranges.values())
    for state in (
      "mhcrown-matched",
      "mhcrown-mismatch",
      "mhcrown-unavailable",
    )
  }
  print(
    "MH Crown comparison: "
    + ", ".join(
      f"{state.removeprefix('mhcrown-')}={count}"
      for state, count in validation_counts.items()
    )
  )

  return {
    "$schema": "../schema/legal-size-ranges.schema.json",
    "sources": {
      "authoritative": "https://mhxx.kiranico.com/en/mon",
      "crossCheck": "https://mhcrown.com/",
    },
    "monsters": dict(sorted(ranges.items())),
  }


def main() -> None:
  parser = argparse.ArgumentParser()
  parser.add_argument("--timeout", type=float, default=15.0)
  parser.add_argument("--retries", type=int, default=2)
  parser.add_argument("--jobs", type=int, default=4)
  args = parser.parse_args()

  ranges = build_ranges(
    max(1.0, args.timeout),
    max(0, min(args.retries, 5)),
    max(1, min(args.jobs, 8)),
  )
  OUTPUT_PATH.write_text(
    json.dumps(ranges, ensure_ascii=False, indent=2) + "\n",
    encoding="utf-8",
  )
  print(f"Wrote {OUTPUT_PATH}")


if __name__ == "__main__":
  main()
