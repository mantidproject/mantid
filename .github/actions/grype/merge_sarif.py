#!/usr/bin/env python3

"""Merge the per-platform grype reports produced by .github/workflows/grype.yml.

Every leg of that workflow's matrix installs the same conda packages for a
different platform, so most vulnerabilities are reported once per platform.
Uploaded separately they list each shared CVE once per platform in code
scanning. This rewrites the reports into a single one, where a finding seen on
several platforms becomes a single result naming the platforms it was found on.

Usage: merge_sarif.py OUTPUT REPORT [REPORT ...]

Each REPORT must be named grype-<platform>.sarif; the label comes from the name.
"""

import json
import pathlib
import sys

# grype scans the environment that pixi installs into the workflow's temporary
# project, so every location it reports sits underneath this directory
SCAN_ROOT_MARKER = "/.pixi/envs/default"
CONDA_META = "/conda-meta/"
REPORT_PREFIX = "grype-"
REPORT_SUFFIX = ".sarif"


def _platform(report: pathlib.Path) -> str:
    name = report.name
    if not name.startswith(REPORT_PREFIX) or not name.endswith(REPORT_SUFFIX):
        raise ValueError(f"cannot read a platform from '{name}', expected {REPORT_PREFIX}<platform>{REPORT_SUFFIX}")

    return name[len(REPORT_PREFIX) : -len(REPORT_SUFFIX)]


def _relative_to_scan_root(uri: str) -> str:
    """Rewrite a location as a path relative to the scanned environment.

    Windows locations arrive looking like
    "D:/a/_temp/grype-scan/.pixi/envs/default/\\conda-meta\\python-3.13.15-h254dcb4_101_cp313.json".
    Grype prepends the scan root to any location that go does not consider
    absolute, and a rooted windows path without a drive letter is not absolute.
    Code scanning rejects the whole report if a location keeps its drive letter,
    reading the "D:" as a URI scheme.
    """
    path = uri.replace("\\", "/")
    root = path.find(SCAN_ROOT_MARKER)
    if root != -1:
        path = path[root + len(SCAN_ROOT_MARKER) :]
    elif len(path) > 1 and path[1] == ":":
        path = path[2:]

    while "//" in path:
        path = path.replace("//", "/")

    return path if path.startswith("/") else f"/{path}"


def _without_conda_build_string(uri: str) -> str:
    """Drop the build string from a conda-meta location.

    The build string differs between platforms - python 3.13.15 is recorded as
    conda-meta/python-3.13.15-hb101c97_101_cp313.json on linux-64 and
    conda-meta/python-3.13.15-h254dcb4_101_cp313.json on win-64 - so it has to
    go for the two to be recognised as the same finding.
    """
    if CONDA_META not in uri or not uri.endswith(".json"):
        return uri

    directory, _, name = uri.rpartition("/")
    package, separator, _build = name[: -len(".json")].rpartition("-")
    if not separator:
        return uri

    return f"{directory}/{package}.json"


def _canonical_locations(result: dict) -> tuple:
    """Canonicalise the result's locations in place, returning them for matching."""
    uris = []
    for location in result.get("locations", []):
        artifact = location.get("physicalLocation", {}).get("artifactLocation", {})
        uri = artifact.get("uri")
        if uri is None:
            continue

        uri = _without_conda_build_string(_relative_to_scan_root(uri))
        artifact["uri"] = uri
        uris.append(uri)

    return tuple(uris)


def _collect(reports: list, rules: dict, results: dict, platforms: dict) -> dict:
    """Read every report, returning the first one as the envelope to merge into."""
    envelope = None

    for report in sorted(reports):
        platform = _platform(report)
        with report.open() as handle:
            sarif = json.load(handle)

        runs = sarif.get("runs", [])
        if envelope is None and runs:
            envelope = sarif

        for run in runs:
            for rule in run.get("tool", {}).get("driver", {}).get("rules", []):
                rules.setdefault(rule["id"], rule)

            for result in run.get("results", []):
                key = (result.get("ruleId"), _canonical_locations(result))
                # grype fingerprints the location it found, which no longer
                # exists once the build string is dropped; leave the fingerprint
                # for code scanning to compute from the canonical location
                result.pop("partialFingerprints", None)
                results.setdefault(key, result)
                platforms.setdefault(key, {})[platform] = None

        print(f"{report.name}: {sum(len(run.get('results', [])) for run in runs)} results on {platform}")

    return envelope


def main() -> int:
    output = pathlib.Path(sys.argv[1])
    reports = [pathlib.Path(argument) for argument in sys.argv[2:]]
    if not reports:
        raise SystemExit("no reports were given to merge")

    rules = {}
    results = {}
    platforms = {}
    envelope = _collect(reports, rules, results, platforms)
    if envelope is None:
        raise SystemExit("the reports contained no runs")

    ordered = sorted(results.items(), key=lambda item: (item[0][0] or "", item[0][1]))
    for key, result in ordered:
        found_on = ", ".join(platforms[key])
        message = result.setdefault("message", {})
        message["text"] = f"{message.get('text', key[0])} (detected on: {found_on})"

    run = envelope["runs"][0]
    run["tool"]["driver"]["rules"] = list(rules.values())
    run["results"] = [result for _key, result in ordered]
    envelope["runs"] = [run]

    with output.open("w") as handle:
        json.dump(envelope, handle, indent=2)

    print(f"{output.name}: {len(run['results'])} merged results, {len(rules)} rules")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
