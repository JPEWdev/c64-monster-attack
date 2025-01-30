#! /usr/bin/env python3
#
# SPDX-License-Identifier: MIT


import argparse
import sys
import xml.etree.ElementTree as ET

from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Convert CST charset to code")
    parser.add_argument("input", type=Path, help="input CST charset")
    parser.add_argument("output", type=Path, help="output assembly file")
    parser.add_argument("--section", help="Output section")

    args = parser.parse_args()

    name = args.input.stem
    chars = {}
    with args.input.open("r") as f:
        tree = ET.parse(f)
    root = tree.getroot()
    if root.tag != "charset":
        print("Input file does not appear to be charset data")
        return 1

    for char in root.iter("character"):
        charcode = int(char.find("charcode").text)
        data = []
        for d in char.find("chardata").iter("data"):
            data.append(int(d.text))
        if len(data) != 8:
            print(
                f"Character {charcode} has wrong number of data bytes. Got {len(data)}, expected 8"
            )
            return 1
        chars[charcode] = data

    with args.output.open("w") as f:
        if args.section:
            f.write(f'    .section {args.section}, "a"\n')
        f.write("    .align 1<<11\n")
        f.write(f"    .global {name}\n")
        f.write(f"{name}:\n")
        for i in range(0, 256):
            f.write("    .byte " + ",".join(f"${c:02X}" for c in chars[i]) + "\n")


    return 0


if __name__ == "__main__":
    sys.exit(main())
