"""
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""

import argparse
import os
import re
import tomllib
import yaml
from pathlib import Path
from subprocess import run
from typing import Sequence, Dict, Tuple, NewType

mantid_root = Path(os.path.dirname(os.path.realpath(__file__))).parent.parent
BUILD_CONFIG_PATH = mantid_root / Path("conda/recipes/conda_build_config.yaml")
MANTID_DEVELOPER_RECIPE_PATH = mantid_root / Path("conda/recipes/mantid-developer/recipe.yaml")
PIXI_TOML = mantid_root / Path("pixi.toml")

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
        if package:
            pin_map[package] = {"all": version}

    for os_section in [entry for entry in runtime_dependencies if isinstance(entry, dict)]:
        os_label = os_section["if"]
        os_names = os_label.split(" or ")
        for pin_string in os_section["then"]:
            package, version = _interpret_recipe_pin(pin_string, os_names[0], conda_build_config_pins)
            if package:
                pin_map.setdefault(package, {})
                for name in os_names:
                    pin_map[package] |= {name: version}

    return pin_map


def _interpret_recipe_pin(pin_string: str, os_label: str, conda_build_config_pins: DependencyPins) -> Tuple[str | None, str | None]:
    jinja_pin_pattern = re.compile(r"([\w|\-|_|\.]+) \${{ ([\w|\-|_|\.]+) }}")
    jinja_match = jinja_pin_pattern.search(pin_string)
    if jinja_match:
        package = jinja_match.group(1)
        pin_label = jinja_match.group(2)
        for os_name, version in conda_build_config_pins[pin_label].items():
            if os_label == os_name or os_name == "all":
                return package, version

    normal_pin_pattern = re.compile(r"([\w|\-|_|\.]+)(\s*([=|!|>|<].*))?")
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

    for dependency_header, os_name in dependency_headers:
        if data := _get_pixi_dependencies_from_header(dependency_header, manifest_data):
            for package, version in data.items():
                pin_map.setdefault(package, {})
                if isinstance(version, str):
                    pin_map[package] |= {os_name: version}
                elif isinstance(version, dict):
                    version_and_build = f"{version['version']} {version['build']}"
                    pin_map[package] |= {os_name: version_and_build}

    return pin_map


def _get_pixi_dependencies_from_header(header: str, manifest_data: Dict):
    sub_headers = header.split(".")
    data = manifest_data
    while sub_headers:
        h = sub_headers.pop(0)
        if h in data:
            data = data[h]
        else:
            return None
    return data


def update_pin_in_pixi_manifest(package: str, os_name: str, version: str):
    os_label_map = {"linux": "linux-64", "win": "win-64", "osx": "osx-arm64"}
    if version and version[0] not in ("=", "!", "<", ">"):
        version = "=" + version
    command = ["pixi", "add", f"{package}{version}"]
    if os_name != "all":
        command += ["--platform", os_label_map[os_name]]
    run(command, check=True)


def _compare_version_numbers(conda_version: str, pixi_version: str):
    trimmed_conda_version = conda_version.replace(" ", "").lstrip("=").replace(".*", "")
    trimmed_pixi_version = pixi_version.replace(" ", "").lstrip("=").replace(".*", "")

    return trimmed_conda_version == trimmed_pixi_version


def main(argv: Sequence[str] = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "filenames",
        nargs="*",
        help="Filenames pre-commit believes are changed.",
    )
    args = parser.parse_args(argv)
    changed_files = [Path(f) for f in args.filenames]

    conda_env = get_mantid_dev_conda_recipe_pins(get_conda_recipe_pins())
    pixi_env = get_pixi_mantid_dev_pins()

    if (BUILD_CONFIG_PATH in changed_files or MANTID_DEVELOPER_RECIPE_PATH in changed_files) and PIXI_TOML not in changed_files:
        for package, conda_versions in conda_env.items():
            pixi_versions = pixi_env.get(package, {})
            for os_name, conda_version in conda_versions.items():
                pixi_version = pixi_versions.get(os_name, "")
                if not pixi_version:
                    update_pin_in_pixi_manifest(package, os_name, conda_version)
                # if you run pixi add with no spec specified, pixi will add some amount of version restraint;
                # therefore, we want to check that the conda version has actually been specified before updating the toml
                if pixi_version and conda_version and not _compare_version_numbers(conda_version, pixi_version):
                    update_pin_in_pixi_manifest(package, os_name, conda_version)


if __name__ == "__main__":
    raise SystemExit(main())
