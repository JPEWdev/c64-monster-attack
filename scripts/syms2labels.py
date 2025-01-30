#! /usr/bin/env python3
#
# SPDX-License-Identifier: MIT

import argparse
import sys

from pathlib import Path


def main():
    parser = argparse.ArgumentParser(
        description="Convert objdump symbols to vice label directives"
    )
    parser.add_argument("input", type=Path, help="Input symbol table in nm format")
    parser.add_argument("output", type=Path, help="Output symbol directives")
    parser.add_argument(
        "-b",
        "--break",
        dest="breaks",
        help="Set initial breakpoint",
        default=[],
        action="append",
    )

    args = parser.parse_args()

    symbols = {}

    with args.input.open("r") as f:
        for line in f:
            addr, typ, name = line.rstrip().split()
            symbols[name] = int(addr, 16)

    with args.output.open("w") as f:
        for name, addr in symbols.items():
            f.write(f"add_label {addr:x} .{name}\n")
        for b in args.breaks:
            f.write(f"break .{b}\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
