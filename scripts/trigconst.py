#! /usr/bin/env python3
#
# SPDX-License-Identifier: MIT

import argparse
import sys
import math

from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Generate Trig constants")
    parser.add_argument("output", help="Output header file", type=Path)

    args = parser.parse_args()

    with args.output.open("w") as f:
        bn = args.output.stem.upper()
        f.write(f"#ifndef _{bn}\n")
        f.write(f"#define _{bn}\n\n")
        for a in range(360):
            rad = a * math.pi / 180
            f.write(f"#define SIN_{a} ({math.sin(rad)})\n")
            f.write(f"#define COS_{a} ({math.cos(rad)})\n")
        f.write(f"\n\n#endif\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
