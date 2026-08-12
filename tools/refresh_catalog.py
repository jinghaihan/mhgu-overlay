#!/usr/bin/env python3
"""Refresh factual monster metadata from the public Kiranico pages.

The seed file contains project-owned localization choices and reverse-engineered
Switch identifiers. This tool only imports names and numeric size thresholds.
It intentionally does not mirror page prose, quest tables, or media.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import re
import time
from dataclasses import dataclass
from decimal import Decimal, ROUND_HALF_UP
from html.parser import HTMLParser
from pathlib import Path
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
SEED_PATH = ROOT / "data" / "catalog" / "monsters.seed.json"
OUTPUT_PATH = ROOT / "data" / "catalog" / "monsters.json"
BASE_URL = "https://mhxx.kiranico.com/en/mon"
USER_AGENT = "mhgu-overlay-catalog/0.1 (+https://github.com/jinghaihan/mhgu-overlay)"


@dataclass
class PageFacts:
  english: str = ""
  japanese: str = ""
  base_size: Decimal | None = None
  mini_size: Decimal | None = None
  silver_size: Decimal | None = None
  gold_size: Decimal | None = None


class MonsterPageParser(HTMLParser):
  def __init__(self) -> None:
    super().__init__()
    self.facts = PageFacts()
    self._heading: str | None = None
    self._heading_text: list[str] = []
    self._in_title = False
    self._in_rt = False
    self._title_ja: list[str] = []
    self._title_en: list[str] = []

  def handle_starttag(
    self,
    tag: str,
    attrs: list[tuple[str, str | None]],
  ) -> None:
    if tag in {"h5", "h6"}:
      self._heading = tag
      self._heading_text = []
    elif tag == "h2" and not self.facts.english:
      self._in_title = True
    elif tag == "rt" and self._in_title:
      self._in_rt = True

  def handle_data(self, data: str) -> None:
    if self._heading:
      self._heading_text.append(data)
    if self._in_title:
      target = self._title_en if self._in_rt else self._title_ja
      target.append(data)

  def handle_endtag(self, tag: str) -> None:
    if tag == "rt":
      self._in_rt = False
    elif tag == "h2" and self._in_title:
      self._in_title = False
      self.facts.japanese = clean_name("".join(self._title_ja))
      self.facts.english = clean_name("".join(self._title_en))
    elif tag == self._heading:
      self._consume_heading("".join(self._heading_text))
      self._heading = None

  def _consume_heading(self, text: str) -> None:
    value = parse_decimal(text)
    if value is None:
      return
    if text.startswith("Size:"):
      self.facts.base_size = value
    elif text.startswith("Small Crown:"):
      self.facts.mini_size = value
    elif text.startswith("Silver Crown:"):
      self.facts.silver_size = value
    elif text.startswith("Gold Crown:"):
      self.facts.gold_size = value


def clean_name(value: str) -> str:
  return (
    value.replace("\N{WARNING SIGN}", "").replace("\N{NO-BREAK SPACE}", " ").strip()
  )


def parse_decimal(value: str) -> Decimal | None:
  match = re.search(r"\d[\d,]*\.\d+", value)
  return Decimal(match.group(0).replace(",", "")) if match else None


def to_x100(value: Decimal | None) -> int:
  if value is None:
    return 0
  return int((value * 100).to_integral_value(rounding=ROUND_HALF_UP))


def as_percent(threshold: Decimal | None, base: Decimal | None) -> int:
  if threshold is None or base is None or base == 0:
    return 0
  return int(((threshold * 100) / base).to_integral_value(rounding=ROUND_HALF_UP))


def fetch_facts(kiranico_id: str) -> PageFacts:
  url = f"{BASE_URL}/{kiranico_id}"
  document = ""
  for attempt in range(3):
    try:
      request = Request(url, headers={"User-Agent": USER_AGENT})
      with urlopen(request, timeout=30) as response:
        document = response.read().decode("utf-8")
      break
    except OSError:
      if attempt == 2:
        raise
      time.sleep(0.5 * (attempt + 1))
  parser = MonsterPageParser()
  parser.feed(document)
  if not parser.facts.english or not parser.facts.japanese:
    raise RuntimeError(f"Could not parse monster names from {url}")
  return parser.facts


def build_catalog(
  delay: float,
  jobs: int,
) -> dict:
  seed = json.loads(SEED_PATH.read_text(encoding="utf-8"))
  rows = seed["monsters"]
  crown_validation_overrides = seed.get("crownValidationOverrides", {})
  fetched: dict[str, PageFacts] = {}
  with ThreadPoolExecutor(max_workers=jobs) as executor:
    futures = {executor.submit(fetch_facts, row[0]): row[0] for row in rows}
    for completed, future in enumerate(as_completed(futures), start=1):
      kiranico_id = futures[future]
      fetched[kiranico_id] = future.result()
      print(f"fetched {completed:02}/{len(rows)}")
      if delay:
        time.sleep(delay)

  monsters = []

  for index, row in enumerate(rows, start=1):
    kiranico_id, key, raw_id = row
    facts = fetched[kiranico_id]
    variable = facts.base_size is not None and all(
      as_percent(value, facts.base_size) > 0
      for value in (
        facts.mini_size,
        facts.silver_size,
        facts.gold_size,
      )
    )
    monsters.append(
      {
        "id": index,
        "key": key,
        "baseSizeX100": to_x100(facts.base_size),
        "crowns": {
          "miniPercent": as_percent(
            facts.mini_size,
            facts.base_size,
          ),
          "silverPercent": as_percent(
            facts.silver_size,
            facts.base_size,
          ),
          "goldPercent": as_percent(
            facts.gold_size,
            facts.base_size,
          ),
        },
        "variableSize": variable,
        "switch": {"rawId": raw_id},
        "sources": {
          "sizeAndNames": f"{BASE_URL}/{kiranico_id}",
          "crownValidation": crown_validation_overrides.get(
            key,
            f"https://mhcrown.com/mhgu/{key.replace('-', '')}/",
          )
          if variable
          else None,
        },
      }
    )
  return {
    "$schema": "../schema/monster-catalog.schema.json",
    "generatedFrom": {
      "seed": "data/catalog/monsters.seed.json",
      "primary": "https://mhxx.kiranico.com/en/mon",
      "validation": "https://mhcrown.com/",
    },
    "monsters": monsters,
  }


def main() -> None:
  parser = argparse.ArgumentParser()
  parser.add_argument(
    "--delay",
    type=float,
    default=0.02,
    help="delay after each completed request in seconds",
  )
  parser.add_argument(
    "--jobs",
    type=int,
    default=4,
    help="maximum concurrent requests",
  )
  args = parser.parse_args()
  catalog = build_catalog(
    max(0.0, args.delay),
    max(1, min(args.jobs, 8)),
  )
  OUTPUT_PATH.write_text(
    json.dumps(catalog, ensure_ascii=False, indent=2) + "\n",
    encoding="utf-8",
  )
  print(f"Wrote {OUTPUT_PATH}")


if __name__ == "__main__":
  main()
