#!/usr/bin/env python3

import json
import os
import pathlib
import sys


def _matcher_path(uri: str, repo_root: pathlib.Path) -> str:
    path = pathlib.Path(uri)
    if path.is_absolute():
        return path.as_posix()

    return (repo_root / path).as_posix()


def main() -> int:
    report_path = pathlib.Path(sys.argv[1])

    with report_path.open() as handle:
        report = json.load(handle)

    repo_root = pathlib.Path(os.environ["GITHUB_WORKSPACE"]).resolve()
    severities = {
        "error": "error",
        "warning": "warning",
        "note": "information",
        "none": "information",
    }

    for run in report.get("runs", []):
        for result in run.get("results", []):
            level = severities.get(result.get("level", "warning"), "warning")
            rule_id = result.get("ruleId", "cppcheck")
            message = result.get("message", {}).get("text", "")

            for location in result.get("locations", []):
                physical_location = location.get("physicalLocation", {})
                artifact_location = physical_location.get("artifactLocation", {})
                region = physical_location.get("region", {})
                uri = artifact_location.get("uri")
                if not uri:
                    continue

                line = region.get("startLine", 1)
                column = region.get("startColumn", 1)
                file_path = _matcher_path(uri, repo_root)
                print(f"{file_path}:{line}:{column}: {level}: {message} [{rule_id}]")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
