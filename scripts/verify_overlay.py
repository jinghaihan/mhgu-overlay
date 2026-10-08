#!/usr/bin/env python3
"""Verify that a built Tesla overlay contains its required NACP metadata."""

from __future__ import annotations

import struct
import sys
from pathlib import Path


EXPECTED_TITLE = "MHGU Overlay"
EXPECTED_AUTHOR = "Jing Haihan"
NACP_SIZE = 0x4000


def fail(message: str) -> None:
  raise SystemExit(f"overlay verification failed: {message}")


def read_string(data: bytes, start: int, size: int) -> str:
  raw = data[start : start + size].split(b"\0", 1)[0]
  return raw.decode("utf-8")


def main() -> None:
  path = Path(sys.argv[1] if len(sys.argv) > 1 else "mhgu-overlay.ovl")
  data = path.read_bytes()
  if len(data) < 0x20 or data[0x10:0x14] != b"NRO0":
    fail("missing NRO0 header")

  nro_size = struct.unpack_from("<I", data, 0x18)[0]
  if (
    nro_size < 0x20
    or nro_size + 0x38 > len(data)
    or data[nro_size : nro_size + 4] != b"ASET"
  ):
    fail("missing ASET asset header")

  nacp_offset, nacp_size = struct.unpack_from("<QQ", data, nro_size + 0x18)
  nacp_start = nro_size + nacp_offset
  nacp_end = nacp_start + nacp_size
  if nacp_offset < 0x38 or nacp_size != NACP_SIZE or nacp_end > len(data):
    fail("missing or truncated NACP asset")

  nacp = data[nacp_start:nacp_end]
  title = read_string(nacp, 0, 0x200)
  author = read_string(nacp, 0x200, 0x100)
  if title != EXPECTED_TITLE or author != EXPECTED_AUTHOR:
    fail(f"unexpected metadata: title={title!r}, author={author!r}")

  print(f"verified Tesla overlay metadata in {path}")


if __name__ == "__main__":
  main()
