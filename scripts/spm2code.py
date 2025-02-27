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
    parser.add_argument(
        "code",
        type=Path,
        help="Output C file",
    )
    parser.add_argument(
        "header",
        type=Path,
        help="Output Header file",
    )

    args = parser.parse_args()

    with args.input.open("r") as f:
        data = json.load(f)

    name = args.input.stem

    with args.code.open("w") as code_f, args.header.open("w") as header_f:
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

        header_f.write(f"#ifndef _{name.upper()}\n")
        header_f.write(f"#define _{name.upper()}\n\n")
        header_f.write('#include "sprite.h"\n')
        header_f.write("#include <stdint.h>\n\n")

        all_flags = []

        num_frames = len(data["sprites"])
        header_f.write(f"#define {name.upper()}_NUM_FRAMES ({num_frames})\n\n")

        code_f.write(f'#include "{args.header.name}"\n')
        code_f.write('#include "sprite.h"\n\n')
        code_f.write("extern uint8_t video_base;\n")
        code_f.write(f'__attribute__((section("video_{name}")))\n')
        code_f.write("__attribute__((aligned(64)))\n")

        header_f.write(
            f"extern const struct sprite_frame {name}_frames[{num_frames}];\n"
        )
        code_f.write(f"const struct sprite_frame {name}_frames[{num_frames}] = {{\n")

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
            else:
                for row_idx, row in enumerate(sprite["pixels"]):
                    for p in range(0, len(row), 8):
                        byte = 0
                        for i in range(0, 8):
                            check_bb(row[p + i], row_idx, p + i)
                            byte |= row[p + i] << i

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

            # code_f.write("    .align 1<<6\n")
            # code_f.write(f"    .global {frame_name}\n")
            # code_f.write(f"{frame_name}:\n")
            code_f.write(
                "    {{" + ", ".join("0x%02X" % b for b in pixel_data) + "}},\n"
            )
            # code_f.write(f"    .byte 0x{flags:02X}\n\n")
            all_flags.append(flags)

        code_f.write("};\n\n")

        header_f.write(f"extern const struct bb {name}_bb;\n\n")
        code_f.write(
            f"const struct bb {name}_bb = {{{north}, {south}, {east}, {west}}};\n\n"
        )

        header_f.write(f"#define {name.upper()}_WIDTH ({east - west})\n")
        header_f.write(f"extern const uint8_t {name}_width;\n\n")
        code_f.write(f"const uint8_t {name}_width = {name.upper()}_WIDTH;\n")

        header_f.write(f"#define {name.upper()}_HEIGHT ({south - north})\n")
        header_f.write(f"extern const uint8_t {name}_height;\n\n")
        code_f.write(f"const uint8_t {name}_height = {name.upper()}_HEIGHT;\n")

        header_f.write(f"extern const uint8_t {name}_flags[{num_frames}];\n\n")
        code_f.write(f"const uint8_t {name}_flags[{num_frames}] = {{")
        code_f.write(", ".join("0x%02X" % f for f in all_flags))
        code_f.write("};\n")

        header_f.write(f"extern uint8_t {name}_pointers[{num_frames}];\n")
        code_f.write(f"uint8_t {name}_pointers[{num_frames}];\n\n")
        code_f.write("__attribute__((constructor)) static void init(void) {\n")
        code_f.write(f"    for(uint8_t i = 0; i < {num_frames}; i++) {{\n")
        code_f.write(f"        {name}_pointers[i] = ((uint16_t)&{name}_frames[i] - (uint16_t)&video_base) / 64;\n")
        code_f.write("    }\n")
        code_f.write("}\n")


        header_f.write(f"#define {name.upper()}_SPRITE {{\\\n")
        header_f.write(f"    {name.upper()}_NUM_FRAMES, \\\n")
        header_f.write(f"    {name}_pointers, \\\n")
        header_f.write(f"    {name}_flags, \\\n")
        header_f.write("    }\n")

        header_f.write("#endif\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
