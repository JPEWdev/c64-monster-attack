#! /usr/bin/env python3
#
# SPDX-License-Identifier: MIT

import yaml
import argparse
import sys
import textwrap

from dataclasses import dataclass
from pathlib import Path


class ColorException(Exception):
    pass


MAP_ROWS = 11
MAP_COLS = 19


COLORS = [
    "black",
    "white",
    "red",
    "cyan",
    "purple",
    "green",
    "blue",
    "yellow",
    "orange",
    "brown",
    "lightred",
    "gray1",
    "gray2",
    "lightgreen",
    "lightblue",
    "gray3",
]


@dataclass
class Tile:
    idx: int
    image: int
    color: int
    passable: bool


def get_color(color):
    if color not in COLORS:
        raise ColorException(f"Unknown color {color}")
    return color.upper()


def get_tile_color(color):
    if color not in COLORS[:8]:
        raise ColorException(f"Unknown tile color {color}")
    return color.upper()


def main():
    parser = argparse.ArgumentParser(description="Convert map data file to code")
    parser.add_argument("input", type=Path, help="Input YAML data file")
    parser.add_argument("output", type=Path, help="Output C file")
    args = parser.parse_args()

    with args.input.open("r") as f:
        map_data = yaml.load(f, Loader=yaml.SafeLoader)

    with args.output.open("w") as f:
        f.write(
            textwrap.dedent(
                """\
                #include <cbm.h>
                #include "map.h"

                """
            )
        )
        for screen in map_data:
            name = screen["name"]
            bg0 = get_color(screen["bg0"])
            bg1 = get_color(screen["bg1"])
            bg2 = get_color(screen["bg2"])
            data = screen["data"].strip().splitlines()
            if len(data) != MAP_ROWS:
                print(f"Expected {MAP_ROWS} rows for {name}. Got {len(data)}")
                return 1

            for idx, row in enumerate(data):
                if len(row) != MAP_COLS:
                    print(
                        f"Expected {MAP_COLS} columns for {name}, row {idx}. Got {len(row)}"
                    )
                    return 1

            legend = {}
            f.write(f"static const uint8_t legend_{name}[] = {{\n")
            for idx, v in enumerate(screen["legend"].items()):
                legend_name, val = v

                if not isinstance(legend_name, str) or len(legend_name) != 1:
                    print(
                        f"Legend key must be a single character string, got {legend_name!r}"
                    )
                    return 1

                image = val["image"].upper()

                color = get_tile_color(val["color"])
                passable = val.get("passable", False)
                legend[legend_name] = Tile(idx, image, color, passable)
                f.write(f"    MAKE_LEGEND(MAP_IMAGE_{image}, COLOR_{color}),\n")
            f.write("};\n")
            f.write(
                textwrap.dedent(
                    f"""\
                    const struct map_screen {name} = {{
                        COLOR_{bg0},
                        COLOR_{bg1},
                        COLOR_{bg2},
                        legend_{name},
                        {{
                    """
                )
            )

            for row_idx, row in enumerate(data):
                f.write(f"        // Row {row_idx}\n")
                f.write("        {\n")
                for col_idx, c in enumerate(row):
                    if c not in legend:
                        print(f"Unknown character {c} in {name}:{row_idx},{col_idx}")
                        return 1
                    v = legend[c].idx
                    if legend[c].passable:
                        v |= 0x80
                    f.write(f"            0x{v:02X},\n")
                f.write("        },\n")

            f.write("   },\n")
            f.write("};\n\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
