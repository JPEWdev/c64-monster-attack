#! /usr/bin/env python3
#
# SPDX-License-Identifier: MIT

import argparse
import json
import sys
import textwrap

from pathlib import Path


def color(c):
    if c == 2:
        return 1
    if c == 1:
        return 2
    return c


SPRITE_IMAGE_EXPAND_X = 1 << 0
SPRITE_IMAGE_EXPAND_Y = 1 << 1
SPRITE_IMAGE_MULTICOLOR = 1 << 2


def main():
    parser = argparse.ArgumentParser(description="Convert SPM file to code")
    parser.add_argument("input", type=Path, help="Input SPM file")
    parser.add_argument("output", type=Path, help="Output C file")
    parser.add_argument(
        "--format",
        help="Choose assembly output format",
        choices=("vasm", "gas"),
        default="vasm",
    )

    args = parser.parse_args()

    with args.input.open("r") as f:
        data = json.load(f)

    name = args.input.stem

    with args.output.open("w") as f:
        frame_names = []

        north = None
        south = None
        east = None
        west = None

        def check_bb(v, row, col):
            nonlocal north
            nonlocal south
            nonlocal east
            nonlocal west

            if not v:
                return

            if north is None or row < north:
                north = row

            if south is None or row > south:
                south = row

            if east is None or col > east:
                east = col

            if west is None or col < west:
                west = col

        for sprite_idx, sprite in enumerate(data["sprites"]):
            pixel_data = []
            if sprite["multicolor"]:
                for row_idx, row in enumerate(sprite["pixels"]):
                    for p in range(0, len(row), 8):
                        check_bb(row[p], row_idx, p)
                        check_bb(row[p], row_idx, p + 1)
                        check_bb(row[p + 2], row_idx, p + 2)
                        check_bb(row[p + 2], row_idx, p + 3)
                        check_bb(row[p + 4], row_idx, p + 4)
                        check_bb(row[p + 4], row_idx, p + 5)
                        check_bb(row[p + 6], row_idx, p + 6)
                        check_bb(row[p + 6], row_idx, p + 7)

                        byte = color(row[p]) << 6
                        byte |= color(row[p + 2]) << 4
                        byte |= color(row[p + 4]) << 2
                        byte |= color(row[p + 6])
                        pixel_data.append(byte)
            frame_name = f"{name}_{sprite_idx}"

            flags = 0
            if sprite["double_x"]:
                flags |= SPRITE_IMAGE_EXPAND_X
                east *= 2
                west *= 2

            if sprite["double_y"]:
                flags |= SPRITE_IMAGE_EXPAND_Y
                north *= 2
                south *= 2

            if sprite["multicolor"]:
                flags |= SPRITE_IMAGE_MULTICOLOR

            if args.format == "vasm":
                f.write(f'    section video_{frame_name}, "adr"\n')
                f.write("    align 6\n")
                f.write(f"    extern _{frame_name}\n")
                f.write(f"_{frame_name}:\n")
                f.write("    BYTE " + ", ".join("$%02X" % b for b in pixel_data) + "\n")
                f.write(f"    BYTE ${flags:02X}\n\n")
            else:
                f.write(f"    .section video_{frame_name}\n")
                f.write("    .align 1<<6\n")
                f.write(f"    .global {frame_name}\n")
                f.write(f"{frame_name}:\n")
                f.write(
                    "    .byte " + ", ".join("0x%02X" % b for b in pixel_data) + "\n"
                )
                f.write(f"    .byte 0x{flags:02X}\n\n")
        f.write("    .section .rodata\n")
        f.write(f"    .global {name}_bb\n")
        f.write(f"{name}_bb:\n")
        f.write(f"    .byte {north}, {south}, {east}, {west}\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
