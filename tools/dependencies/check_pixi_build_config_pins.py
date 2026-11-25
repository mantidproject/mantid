# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Report packages whose version pin in pixi.toml differs from conda/recipes/conda_build_config.yaml.

Only packages pinned in both files are compared, for each platform. Nothing is modified.
Exits with 1 if any pins differ.
"""

import re
import sys
import tomllib
from pathlib import Path

BUILD_CONFIG = Path("conda/recipes/conda_build_config.yaml")
PIXI_TOML = Path("pixi.toml")

PLATFORMS = ("linux", "osx", "win")
PIXI_PLATFORMS = {"linux-64": "linux", "osx-arm64": "osx", "win-64": "win"}

# pixi package names whose pin lives under a different key in conda_build_config.yaml.
# Any other package is looked up by its name with "-" replaced by "_".
PIXI_TO_BUILD_CONFIG_KEY = {
    "clang_osx-arm64": "c_compiler_version",
    "clangxx_osx-arm64": "cxx_compiler_version",
    "gxx_linux-64": "cxx_compiler_version",
    "libboost-devel": "libboost",
    "libboost-python-devel": "libboost",
    "matplotlib-base": "matplotlib",
    "myst-parser": "mystparser",
    "qtconsole-base": "qtconsole",
    "tbb-devel": "tbb",
}

SELECTOR = re.compile(r"#\s*\[(.+?)\]")

# {name: {"all" | platform: pin}}
Pins = dict[str, dict[str, str]]


def _platforms(selector: str | None) -> list[str]:
    if selector is None:
        return ["all"]
    names = []
    for name in selector.split(" or "):
        names += ["linux", "osx"] if name.strip() == "unix" else [name.strip()]
    return names


def read_build_config(path: Path = BUILD_CONFIG) -> Pins:
    """Parse line by line, because the platform selectors are YAML comments."""
    pins: Pins = {}
    key: str = ""
    key_selector: str | None = None
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        selector = SELECTOR.search(line)
        if stripped.startswith("-"):
            value = stripped[1:].split("#")[0].strip().strip("'\"")
            for platform in _platforms(selector.group(1) if selector else key_selector):
                pins[key][platform] = value
        else:
            key = stripped.split(":")[0].strip()
            key_selector = selector.group(1) if selector else None
            pins[key] = {}
    return pins


def read_pixi(path: Path = PIXI_TOML) -> Pins:
    """Pins of the default environment only; features define other environments."""
    manifest = tomllib.loads(path.read_text())
    sections = [("all", manifest.get("dependencies", {}))]
    for pixi_platform, platform in PIXI_PLATFORMS.items():
        sections.append((platform, manifest.get("target", {}).get(pixi_platform, {}).get("dependencies", {})))

    pins: Pins = {}
    for platform, dependencies in sections:
        for package, spec in dependencies.items():
            if isinstance(spec, dict):
                spec = f"{spec.get('version', '')} {spec.get('build', '')}"
            pins.setdefault(package, {})[platform] = spec
    return pins


def _normalise(pin: str) -> str:
    return pin.replace(" ", "")


def find_mismatches(build_config: Pins, pixi: Pins) -> list[str]:
    messages = []
    for package, pixi_pins in sorted(pixi.items()):
        config_pins = build_config.get(PIXI_TO_BUILD_CONFIG_KEY.get(package, package.replace("-", "_")))
        if not config_pins:
            continue
        for platform in PLATFORMS:
            pixi_pin = pixi_pins.get(platform, pixi_pins.get("all"))
            config_pin = config_pins.get(platform, config_pins.get("all"))
            if pixi_pin is not None and config_pin is not None and _normalise(pixi_pin) != _normalise(config_pin):
                messages.append(f"{package} ({platform}): '{pixi_pin}' in {PIXI_TOML}, '{config_pin}' in {BUILD_CONFIG}")
    return messages


def main() -> int:
    if mismatches := find_mismatches(read_build_config(), read_pixi()):
        print(f"Version pins differ between {PIXI_TOML} and {BUILD_CONFIG}:")
        print("\n".join(mismatches))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
