import re
import sys
import urllib.request


def dependency_spotter(first_build: int, second_build: int, os_name: str = "linux-64"):
    # Form URLs for each build artifact file
    first_build_output_path = form_url_for_build_artifact(first_build, os_name)
    second_build_output_path = form_url_for_build_artifact(second_build, os_name)

    # Read in the packages used, with versions
    first_output_packages = extract_package_versions(first_build_output_path, os_name)
    second_output_packages = extract_package_versions(second_build_output_path, os_name)

    # Next we compare the versions of each package, or say if a package has been added/removed
    packages_added = []
    packages_removed = []
    packages_changed = {}
    for package in first_output_packages:
        if second_output_packages[package] is None:
            packages_removed.extend(package)
        elif second_output_packages[package] != first_output_packages[package]:
            packages_changed[package] = first_output_packages[package] + "  ->  " + second_output_packages[package]
    for package in second_output_packages:
        if first_output_packages[package] is None:
            packages_added.extend(package)

    # Output
    if len(packages_added) > 0:
        print("Packages added:")
        for p in packages_added:
            print(p)
        print("")
    if len(packages_removed) > 0:
        print("Packages removed:")
        for p in packages_removed:
            print(p)
        print("")
    if len(packages_changed) > 0:
        print("Packages changed:")
        for p in packages_changed:
            print(p + " changed from " + packages_changed[p])


def form_url_for_build_artifact(build_number: int, os_name: str):
    return (
        "https://builds.mantidproject.org/job/main_nightly_deployment_prototype/"
        + str(build_number)
        + "/artifact/conda-bld/"
        + os_name
        + "/env_logs/mantidworkbench_build_environment.txt"
    )


def extract_package_versions(url: str, os_name: str) -> dict:
    regex_pattern = os_name + "\/([\w\-]+)-([\w\-.]+)\.(conda|tar\.bz2)"  # noqa: W605
    package_version_dict = {}
    with urllib.request.urlopen(url) as file:
        for line in file.readlines():
            regex_result = re.search(pattern=regex_pattern, string=line.decode("utf-8"))
            if regex_result is not None and len(regex_result.groups()) == 3:
                package_name = regex_result.group(1)
                version = regex_result.group(2)
                package_version_dict[package_name] = version
    return package_version_dict


if __name__ == "__main__":
    if len(sys.argv) == 4:
        dependency_spotter(int(sys.argv[1]), int(sys.argv[2]), sys.argv[3])
    else:
        dependency_spotter(int(sys.argv[1]), int(sys.argv[2]))
