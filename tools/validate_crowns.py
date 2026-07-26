#!/usr/bin/env python3
"""Cross-check Kiranico crown thresholds against mhcrown's factual sizes."""

from __future__ import annotations

import argparse
import json
import re
import time
from decimal import Decimal
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
CATALOG_PATH = ROOT / "data" / "catalog" / "monsters.json"
USER_AGENT = "mhgu-overlay-catalog/0.1 (+https://github.com/jinghaihan/mhgu-overlay)"


def formatted_size(base_x100: int, percent: int) -> str:
    value = (Decimal(base_x100) * Decimal(percent) / Decimal(10000))
    return f"{value.quantize(Decimal('0.01'))}"


def page_numbers(url: str) -> set[str]:
    request = Request(url, headers={"User-Agent": USER_AGENT})
    with urlopen(request, timeout=30) as response:
        document = response.read().decode("utf-8")
    return {
        value.replace(",", "")
        for value in re.findall(r"\b\d{3,5}(?:,\d{3})*\.\d{2}\b", document)
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--delay", type=float, default=0.1)
    parser.add_argument(
        "--strict",
        action="store_true",
        help="treat unavailable mhcrown pages as failures",
    )
    args = parser.parse_args()

    catalog = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
    failed = []
    skipped = []
    checked = 0
    for monster in catalog["monsters"]:
        if not monster["variableSize"]:
            continue
        url = monster["sources"]["crownValidation"]
        expected = {
            formatted_size(
                monster["baseSizeX100"],
                monster["crowns"]["miniPercent"],
            ),
            formatted_size(
                monster["baseSizeX100"],
                monster["crowns"]["goldPercent"],
            ),
        }
        try:
            actual = page_numbers(url)
        except (HTTPError, URLError, TimeoutError) as error:
            skipped.append((monster["key"], str(error)))
            continue
        missing = expected - actual
        if missing:
            failed.append((monster["key"], sorted(missing), url))
        else:
            checked += 1
        if args.delay:
            time.sleep(max(0.0, args.delay))

    print(f"validated: {checked}, mismatched: {len(failed)}, unavailable: {len(skipped)}")
    for key, values, url in failed:
        print(f"MISMATCH {key}: {', '.join(values)} ({url})")
    for key, error in skipped:
        print(f"SKIP {key}: {error}")
    if failed or (args.strict and skipped):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
