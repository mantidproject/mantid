#!/usr/bin/env python3
"""Print the CMake generator a configure preset resolves to, following its inherits chain.

Used by the build script to notice when a preset's generator differs from the one that created
an existing build directory. Prints nothing if the preset sets no generator, and exits non-zero
if the preset does not exist.

Usage: preset-generator.py <CMakePresets.json> <preset name>
"""

import json
import sys


def resolve_generator(presets: dict, name: str) -> str:
    preset = presets[name]
    if "generator" in preset:
        return preset["generator"]
    parents = preset.get("inherits", [])
    if isinstance(parents, str):
        parents = [parents]
    # CMake gives earlier entries in inherits precedence
    for parent in parents:
        generator = resolve_generator(presets, parent)
        if generator:
            return generator
    return ""


def main(argv: list) -> int:
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    with open(argv[1]) as handle:
        presets = {preset["name"]: preset for preset in json.load(handle)["configurePresets"]}
    if argv[2] not in presets:
        print(f"Unknown configure preset: {argv[2]}", file=sys.stderr)
        return 1
    print(resolve_generator(presets, argv[2]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
