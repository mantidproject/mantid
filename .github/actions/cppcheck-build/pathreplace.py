#!/usr/bin/env python3

import json
import os
import pathlib
import sys


def _relativize(uri: str, workspace: pathlib.Path) -> str:
    path = pathlib.Path(uri)
    if not path.is_absolute():
        return uri

    try:
        return path.resolve().relative_to(workspace).as_posix()
    except ValueError:
        return uri


def main() -> int:
    report_path = pathlib.Path(sys.argv[1])
    workspace = pathlib.Path(os.environ["GITHUB_WORKSPACE"]).resolve()

    with report_path.open() as handle:
        report = json.load(handle)

    for run in report.get("runs", []):
        for artifact in run.get("artifacts", []):
            location = artifact.get("location", {})
            uri = location.get("uri")
            if uri:
                location["uri"] = _relativize(uri, workspace)

        for result in run.get("results", []):
            for location in result.get("locations", []):
                artifact_location = location.get("physicalLocation", {}).get("artifactLocation", {})
                uri = artifact_location.get("uri")
                if uri:
                    artifact_location["uri"] = _relativize(uri, workspace)

    with report_path.open("w") as handle:
        json.dump(report, handle)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
