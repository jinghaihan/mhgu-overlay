#!/usr/bin/env python3
"""Offline tests for the MH Crown size parser."""

from decimal import Decimal
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.generate_size_reference import parse_sizes, section  # noqa: E402


class MhCrownParserTests(unittest.TestCase):
  def test_extracts_only_the_paragraph_after_the_requested_heading(self) -> None:
    document = """
      <p class="menuLine">Miniature Crown</p>
      <p><span class="size">790.77</span><br>Mini quest</p>
      <p>Advertisement 999.99</p>
      <p class="menuLine">Gold Crown</p>
      <p><span class="size">1123.25</span><br>Gold quest</p>
    """

    self.assertEqual(
      [size.value for size in parse_sizes(section(document, "Miniature Crown"))],
      [Decimal("790.77")],
    )
    self.assertEqual(
      [size.value for size in parse_sizes(section(document, "Gold Crown"))],
      [Decimal("1123.25")],
    )

  def test_accepts_malformed_size0_markup_from_yian_kut_ku(self) -> None:
    fragment = """
      <span class="size0">790.77</strong></span><br>
      <span class="size">799.75</span><br>
      <span class="size">808.74</span><br>
    """

    self.assertEqual(
      [size.value for size in parse_sizes(fragment)],
      [Decimal("790.77"), Decimal("799.75"), Decimal("808.74")],
    )

  def test_accepts_plain_strong_markup_from_duramboros(self) -> None:
    fragment = """
      <strong>2563.57</strong><br>
      <span class="size">2584.41</span><br>
      <span class="size0">2605.25</span>
    """

    self.assertEqual(
      [size.value for size in parse_sizes(fragment)],
      [Decimal("2563.57"), Decimal("2584.41"), Decimal("2605.25")],
    )

  def test_preserves_special_size_markers(self) -> None:
    fragment = """
      <span class="size0">1275.70</span> (DLC only)<br>
      <span class="size0">629.61</span> (規格外)<br>
      <span class="size">996.64</span>
    """
    parsed = parse_sizes(fragment)

    self.assertEqual(
      [(size.value, size.special_only) for size in parsed],
      [
        (Decimal("1275.70"), True),
        (Decimal("629.61"), True),
        (Decimal("996.64"), False),
      ],
    )

  def test_ignores_numbers_outside_visible_size_text(self) -> None:
    fragment = """
      <a href="https://example.invalid/video?start=1234.56">Quest</a><br>
      <span class="size">1,275.70</span>
    """

    self.assertEqual(
      [size.value for size in parse_sizes(fragment)],
      [Decimal("1275.70")],
    )


if __name__ == "__main__":
  unittest.main()
