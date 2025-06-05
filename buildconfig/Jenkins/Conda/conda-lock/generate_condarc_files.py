import argparse
from pathlib import Path
from os import path
from enum import Enum
import re


class envType(Enum):
    BUILD = 1
    HOST = 2


class buildPackage(Enum):
    MANTID = 1
    MANTIDQT = 2
    MANTIDWORKBENCH = 3


ENV_TYPE_MAP = {envType.BUILD: "build", envType.HOST: "host"}
BUILD_PACKAGE_MAP = {buildPackage.MANTID: "mantid", buildPackage.MANTIDQT: "mantid", buildPackage.MANTIDWORKBENCH: "mantidworkbench"}


def generate_conda_rc(build_package: buildPackage, env_type: envType, dir: Path, platform: str):
    lockfile = dir / f"{platform}" / f"{BUILD_PACKAGE_MAP[build_package]}-{ENV_TYPE_MAP[env_type]}-lockfile.yml"
    output = dir / f"{platform}" / f"{BUILD_PACKAGE_MAP[build_package]}-{ENV_TYPE_MAP[env_type]}.condarc.yaml"
    if path.exists(lockfile):
        parsed_packages = parse_package_requirements(lockfile)
        generate_file(parsed_packages, output)
        return
    print(f"Lockfile not found: {lockfile}")


def generate_file(parsed_packages, output_file):
    with open(output_file, "w") as f:
        lines_to_write = ["pinned_packages:"]
        lines_to_write.extend([f"  - {k}=={v}" for k, v in parsed_packages.items()])
        f.writelines(lines_to_write)


def parse_package_requirements(lockfile: Path) -> dict[str, str]:
    parsed_packages = {}
    with open(lockfile, "r") as file:
        package_name = None
        for line in file:
            if "- name:" in line:
                match = re.search(r"- name:\s+(.+)", line)
                package_name = match.group(1) if match else None
            if package_name and "version:" in line:
                match = re.search(r"version:\s+[']?(.+)[']?", line)
                parsed_packages[package_name] = match.group(1)
                package_name = None
    return parsed_packages


def parse_arguments():
    parser = argparse.ArgumentParser(description="parser to take arguments to generate conda rc")
    parser.add_argument("-S", dest="conda_lock_dir")
    parser.add_argument("-P", dest="platform")
    parser.add_argument("--build-mantid", dest="build_mantid")
    parser.add_argument("--build-mantidqt", dest="build_mantidqt")
    parser.add_argument("--build-workbench", dest="build_workbench")
    parser.set_defaults(build_mantid=False, build_mantidqt=False, build_workbench=False)
    return parser.parse_args()


def main():
    args = parse_arguments()
    lockfile_dir = Path(args.conda_lock_dir)
    if args.build_mantid:
        generate_conda_rc(buildPackage.MANTID, envType.BUILD, lockfile_dir, args.platform)
        generate_conda_rc(buildPackage.MANTID, envType.HOST, lockfile_dir, args.platform)
    if args.build_mantidqt:
        generate_conda_rc(buildPackage.MANTIDQT, envType.BUILD, lockfile_dir, args.platform)
        generate_conda_rc(buildPackage.MANTIDQT, envType.HOST, lockfile_dir, args.platform)
    if args.build_workbench:
        generate_conda_rc(buildPackage.MANTIDWORKBENCH, envType.BUILD, lockfile_dir, args.platform)
        generate_conda_rc(buildPackage.MANTIDWORKBENCH, envType.HOST, lockfile_dir, args.platform)


if __name__ == "__main__":
    main()
