"""
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""

import argparse
import re
import tomllib
import yaml
from pathlib import Path
from typing import Sequence, Dict, Tuple, NewType

BUILD_CONFIG_PATH = Path("conda/recipes/conda_build_config.yaml")
MANTID_DEVELOPER_RECIPE_PATH = Path("conda/recipes/mantid-developer/recipe.yaml")
PIXI_TOML = Path("pixi.toml")

# package name: {'linux' : '>3.0', 'osx': '>3.1.0'}
# package name: {'all': '==3.2.1'}
DependencyPins = NewType("DependencyPins", Dict[str, Dict[str, str]])


def get_conda_recipe_pins() -> DependencyPins:
    pin_map = {}
    current_package = None
    current_os_selector = None

    os_selector_pattern = re.compile(r"#\s*\[(.+?)\]")
    with open(BUILD_CONFIG_PATH) as file:
        for line in file:
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            if stripped == "pin_run_as_build:":
                break

            if not stripped.startswith("-"):
                # new dependency
                current_package = stripped.split(":")[0].strip()
                pin_map.setdefault(current_package, {})
                match = os_selector_pattern.search(line)
                current_os_selector = match.group(1) if match else None
            else:
                # pin value
                value = stripped.lstrip("-").split("#")[0].strip().strip("'")
                match = os_selector_pattern.search(line)
                os_selector = match.group(1) if match else current_os_selector
                if os_selector is not None:
                    for os_name in os_selector.split(" or "):
                        pin_map[current_package] |= {os_name: value}
                else:
                    pin_map[current_package] |= {"all": value}

    return pin_map


def get_mantid_dev_conda_recipe_pins(conda_build_config_pins: DependencyPins) -> DependencyPins:
    pin_map = {}
    with open(MANTID_DEVELOPER_RECIPE_PATH) as file:
        runtime_dependencies = yaml.safe_load(file)["requirements"]["run"]

    for pin_string in [entry for entry in runtime_dependencies if isinstance(entry, str)]:
        package, version = _interpret_recipe_pin(pin_string, "all", conda_build_config_pins)
        if package and version:
            pin_map[package] = {"all": version}

    for os_section in [entry for entry in runtime_dependencies if isinstance(entry, dict)]:
        os_label = os_section["if"]
        os_names = os_label.split(" or ")
        for pin_string in os_section["then"]:
            package, version = _interpret_recipe_pin(pin_string, os_names[0], conda_build_config_pins)
            if package and version:
                pin_map.setdefault(package, {})
                for name in os_names:
                    pin_map[package] |= {name: version}

    return pin_map


def _interpret_recipe_pin(pin_string: str, os: str, conda_build_config_pins: DependencyPins) -> Tuple[str | None, str | None]:
    jinja_pin_pattern = re.compile(r"([\w|\-|_]+) \${{ ([\w|\-|_]+) }}")
    jinja_match = jinja_pin_pattern.search(pin_string)
    if jinja_match:
        package = jinja_match.group(1)
        pin_label = jinja_match.group(2)
        for os_name, version in conda_build_config_pins[pin_label].items():
            if os == os_name:
                return package, version

    normal_pin_pattern = re.compile(r"([\w|\-|_]+)(\s*([=|!|>|<].*))?")
    pin_match = normal_pin_pattern.search(pin_string)
    if pin_match:
        package = pin_match.group(1)
        version = pin_match.group(3)
        return package, version if version is not None else ""

    return None, None


def get_pixi_mantid_dev_pins() -> DependencyPins:
    pin_map = {}
    manifest_data = {}
    dependency_headers = (
        ("dependencies", "all"),
        ("target.linux-64.dependencies", "linux"),
        ("target.win-64.dependencies", "win"),
        ("target.osx-arm64.dependencies", "osx"),
    )

    with open(PIXI_TOML, "rb") as file:
        manifest_data = tomllib.load(file)

    for dependency_header, os in dependency_headers:
        if dependency_header in manifest_data:
            for package, version in manifest_data.get(dependency_header, {}).items():
                pin_map.setdefault(package, {})
                pin_map[package] |= {os: version}

    return pin_map


def main(argv: Sequence[str] = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "filenames",
        nargs="*",
        help="Filenames pre-commit believes are changed.",
    )
    args = parser.parse_args(argv)
    changed_files = args.filenames

    conda_env = get_mantid_dev_conda_recipe_pins(get_conda_recipe_pins())
    pixi_env = get_pixi_mantid_dev_pins()

    if (BUILD_CONFIG_PATH in changed_files or MANTID_DEVELOPER_RECIPE_PATH in changed_files) and PIXI_TOML not in changed_files:
        for package, versions in conda_env:
            if pixi_env[package] != versions:
                pass


if __name__ == "__main__":
    raise SystemExit(main())
