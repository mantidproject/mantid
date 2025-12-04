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
import subprocess
import tomllib
import yaml
from pathlib import Path
from subprocess import run
from typing import Sequence, Dict, Tuple, NewType, List

BUILD_CONFIG_PATH = Path("conda/recipes/conda_build_config.yaml")
MANTID_DEVELOPER_RECIPE_PATH = Path("conda/recipes/mantid-developer/recipe.yaml")
PIXI_TOML = Path("pixi.toml")

# package name: {'linux' : '>3.0', 'osx': '>3.1.0'}
# package name: {'all': '==3.2.1'}
# occt: {'all': '7.* novtk*'} (example for which uses a build number, just stores the version string from conda)
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

            # assumes this will remain at the end of the file
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
    _run_pixi_command(command)


def remove_from_pixi_manifest(package: str, os_name: str):
    os_label_map = {"linux": "linux-64", "win": "win-64", "osx": "osx-arm64"}
    command = ["pixi", "remove", package]
    if os_name != "all":
        command += ["--platform", os_label_map[os_name]]
    _run_pixi_command(command)


def _run_pixi_command(command: List[str]):
    process = run(command, check=True, text=True, stderr=subprocess.PIPE)
    if process.returncode != 0:
        print(f'Tried to run "{"".join(command)}" but encountered a problem:')
        print(process.stderr)


def _compare_version_numbers(conda_version: str, pixi_version: str):
    trimmed_conda_version = conda_version.replace(" ", "").lstrip("=").rstrip(".*")
    trimmed_pixi_version = pixi_version.replace(" ", "").lstrip("=").rstrip(".*")

    return trimmed_conda_version == trimmed_pixi_version


def update_pixi_from_conda_changes(conda_env: DependencyPins, pixi_env: DependencyPins) -> bool:
    # search for any removed dependencies
    made_change_to_pixi = False
    for package, pixi_versions in pixi_env.items():
        if package not in conda_env.keys():
            remove_from_pixi_manifest(package, "all")
            made_change_to_pixi = True
        else:
            conda_versions = conda_env.get(package, {})
            for os_name in pixi_versions.keys():
                if os_name not in conda_versions.keys():
                    remove_from_pixi_manifest(package, os_name)
                    made_change_to_pixi = True
    # update changed pins or added dependencies
    for package, conda_versions in conda_env.items():
        pixi_versions = pixi_env.get(package, {})
        for os_name, conda_version in conda_versions.items():
            pixi_version = pixi_versions.get(os_name, "")
            if not pixi_version:
                update_pin_in_pixi_manifest(package, os_name, conda_version)
                made_change_to_pixi = True
            # if you run pixi add with no spec specified, pixi will add some amount of version restraint;
            # therefore, we want to check that the conda version has actually been specified before updating the toml
            if pixi_version and conda_version and not _compare_version_numbers(conda_version, pixi_version):
                update_pin_in_pixi_manifest(package, os_name, conda_version)
                made_change_to_pixi = True

    return made_change_to_pixi


def compare_conda_and_pixi_envs(conda_env: DependencyPins, pixi_env: DependencyPins) -> Sequence[str]:
    extra_in_conda = set(conda_env.keys()) - set(pixi_env.keys())
    extra_in_pixi = set(pixi_env.keys()) - set(conda_env.keys())
    messages = []
    if extra_in_conda:
        messages.append("Packages in the conda environment not found within pixi:\n" + "\n".join(extra_in_conda))
        return messages
    if extra_in_pixi:
        messages.append("Packages in the pixi environment not found within conda:\n" + "\n".join(extra_in_pixi))
        return messages

    for package, conda_versions in conda_env.items():
        pixi_versions = pixi_env.get(package, {})
        for os_name, conda_version in conda_versions.items():
            pixi_version = pixi_versions.get(os_name, "")
            if not pixi_version:
                messages.append(f"{package} found for os: {os_name} in the conda environment but not in the pixi environment")
            if pixi_version and conda_version and not _compare_version_numbers(conda_version, pixi_version):
                messages.append(
                    f"{package}{conda_version} found for os: {os_name} in the conda environment but has version pin {pixi_version}"
                    f" in the pixi environment"
                )
        for os_name, pixi_version in pixi_versions.items():
            conda_version = conda_versions.get(os_name, None)
            if conda_version is None:
                messages.append(f"{package} found for os: {os_name} in the pixi environment but not in the conda environment")
            if conda_version and not _compare_version_numbers(conda_version, pixi_version):
                messages.append(
                    f"{package}{pixi_version} found for os: {os_name} in the pixi environment but has version pin {conda_version}"
                    f" in the conda environment"
                )

    return messages


def main(argv: Sequence[str] = None) -> int:
    parser = argparse.ArgumentParser(
        description="""
    This script is intended to be run by pre-commit.
    It takes as arguments the paths of the files changes by the commit.
    If changes have been made to the version pinnings / required packages for mantid-developer
    (as listed in conda/recipes/conda_build_config.yaml and conda/recipe/mantid-developer/recipe.yaml)
    then those changes will be added to the pixi manifest file.

    In cases where the pixi manifest has been updated (or both the manifest and the conda recipe have been updated)
    then the two environments will be compared and the pre-commit will fail if they mismatch.
    """
    )
    parser.add_argument(
        "filenames",
        nargs="*",
        help="Filenames pre-commit believes are changed.",
    )
    args = parser.parse_args(argv)
    changed_files = [Path(f) for f in args.filenames]

    print(changed_files)

    conda_env = get_mantid_dev_conda_recipe_pins(get_conda_recipe_pins())
    pixi_env = get_pixi_mantid_dev_pins()

    if (BUILD_CONFIG_PATH in changed_files or MANTID_DEVELOPER_RECIPE_PATH in changed_files) and PIXI_TOML not in changed_files:
        if update_pixi_from_conda_changes(conda_env, pixi_env):
            return 1

    if warning_messages := compare_conda_and_pixi_envs(conda_env, pixi_env):
        print("Mismatch between conda and pixi environments found:")
        print("\n".join(warning_messages))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
