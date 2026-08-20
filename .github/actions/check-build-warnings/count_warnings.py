#!/usr/bin/env python3

"""Fail a CI job when the C++ compiler emitted warnings during the build.

The build itself is allowed to run to completion; its merged stdout/stderr is
captured to a log file which this script scans afterwards. Only warnings that a
developer can act on are counted, i.e. those attributed to a source file that is
tracked in the repository. Warnings from third-party or generated code, and
warnings without a source location, are reported but do not fail the job.
"""

import argparse
import os
import pathlib
import re
import sys
from collections import Counter

# Colour codes are disabled in CI via COLORED_COMPILER_OUTPUT=OFF, but strip them
# anyway so the script also works on a log captured from a developer build.
ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")

# gcc/clang diagnostics of the form "path:line[:column]: warning: message"
LOCATED_WARNING_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):(?:(?P<column>\d+):)?\s+warning:\s+(?P<message>.*)$")

# Warnings carrying no source location, e.g. "cc1plus: warning: ..." or "ld: warning: ..."
UNLOCATED_WARNING_RE = re.compile(r"^(?P<tool>[^\s:]+):\s+warning:\s+(?P<message>.*)$")

# Trailing warning flag, e.g. the "-Wconversion" in "... [-Wconversion]"
WARNING_FLAG_RE = re.compile(r"\[(-W[^\]]+)\]\s*$")

# The C/C++ extensions listed in .github/filters.yaml, plus those the build can emit
SOURCE_SUFFIXES = frozenset((".c", ".cc", ".cpp", ".cu", ".cxx", ".h", ".hpp", ".hxx", ".tcc"))

# Workspace subdirectories holding generated, vendored or third-party sources
EXCLUDED_TOP_LEVEL_DIRS = frozenset(("build", "_deps", ".pixi"))


def _source_path(raw_path: str, build_dir: pathlib.Path, workspace: pathlib.Path) -> pathlib.Path | None:
    """Return the workspace-relative path of a warning that should be counted, else None.

    Relative paths are resolved against the build directory as that is the working
    directory of the build tool that emitted them.
    """
    candidate = pathlib.Path(raw_path)
    if candidate.suffix.lower() not in SOURCE_SUFFIXES:
        return None

    if not candidate.is_absolute():
        candidate = build_dir / candidate

    resolved = candidate.resolve()
    if not resolved.is_relative_to(workspace):
        return None

    relative = resolved.relative_to(workspace)
    if relative.parts[0] in EXCLUDED_TOP_LEVEL_DIRS:
        return None

    return relative


def _scan(log_path: pathlib.Path, build_dir: pathlib.Path, workspace: pathlib.Path) -> tuple[list, list, int]:
    """Return the deduplicated counted warnings, the uncounted warning lines and the number of lines read."""
    counted = {}
    uncounted = []
    lines_read = 0

    with log_path.open(errors="replace") as handle:
        for raw_line in handle:
            lines_read += 1
            line = ANSI_RE.sub("", raw_line.rstrip("\r\n"))
            match = LOCATED_WARNING_RE.match(line)
            if match:
                relative = _source_path(match["file"], build_dir, workspace)
                if relative is None:
                    uncounted.append(line)
                else:
                    # A warning in a header is re-emitted once per translation unit
                    # that includes it, so key on the location and message.
                    key = (relative.as_posix(), int(match["line"]), int(match["column"] or 0), match["message"])
                    counted.setdefault(key, None)
            elif UNLOCATED_WARNING_RE.match(line):
                uncounted.append(line)

    return sorted(counted), uncounted, lines_read


def _format(warning: tuple) -> str:
    file_path, line, column, message = warning
    location = f"{file_path}:{line}:{column}" if column else f"{file_path}:{line}"

    return f"{location}: warning: {message}"


def _warning_flag(message: str) -> str:
    match = WARNING_FLAG_RE.search(message)

    return match.group(1) if match else "unflagged"


def _write_step_summary(warnings: list) -> None:
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_path:
        return

    lines = [f"## {len(warnings)} compiler warning(s)", "", "```"]
    lines.extend(_format(warning) for warning in warnings)
    lines.extend(("```", ""))
    with open(summary_path, "a") as handle:
        handle.write("\n".join(lines))


def _parse_args(argv: list) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Fail a CI job when the C++ compiler emitted warnings.")
    parser.add_argument("log_file", type=pathlib.Path, help="Build log to scan")
    parser.add_argument(
        "--build-dir",
        type=pathlib.Path,
        help="Directory the build tool ran in, used to resolve relative paths (default: <workspace>/build)",
    )
    parser.add_argument(
        "--allowed-warnings",
        type=int,
        default=0,
        metavar="N",
        help="Number of warnings tolerated before the job fails. Should always be 0; a non-zero value is a "
        "temporary measure so that a known backlog does not break every build while it is dealt with.",
    )

    return parser.parse_args(argv)


def main() -> int:
    args = _parse_args(sys.argv[1:])
    log_path = args.log_file
    workspace = pathlib.Path(os.environ.get("GITHUB_WORKSPACE", os.getcwd())).resolve()
    build_dir = args.build_dir.resolve() if args.build_dir else workspace / "build"

    if args.allowed_warnings < 0:
        print(f"::error::--allowed-warnings must not be negative, got {args.allowed_warnings}")
        return 1

    if not log_path.exists():
        print(f"::error::Build log {log_path} does not exist, cannot check for compiler warnings")
        return 1

    counted, uncounted, lines_read = _scan(log_path, build_dir, workspace)

    # An empty log means the build output was not captured, which must not pass silently
    if lines_read == 0:
        print(f"::error::Build log {log_path} is empty, cannot check for compiler warnings")
        return 1

    if uncounted:
        print("::group::Warnings not counted (third-party, generated code, or no source location)")
        for line in uncounted:
            print(line)
        print("::endgroup::")

    if not counted:
        print("No compiler warnings found")
        return 0

    print(f"::group::{len(counted)} compiler warning(s)")
    for warning in counted:
        print(_format(warning))
    print("::endgroup::")

    flags = Counter(_warning_flag(message) for _, _, _, message in counted)
    print("Warnings by flag:")
    for flag, count in flags.most_common():
        print(f"  {count:5d}  {flag}")

    _write_step_summary(counted)

    if len(counted) <= args.allowed_warnings:
        print(
            f"::warning::Build produced {len(counted)} compiler warning(s), within the allowance of "
            f"{args.allowed_warnings}. Fix these and lower the allowance."
        )
        return 0

    allowance = f" Only {args.allowed_warnings} are allowed." if args.allowed_warnings else ""
    print(f"::error::Build produced {len(counted)} compiler warning(s).{allowance} See the build log artifact for full context.")

    return 1


if __name__ == "__main__":
    raise SystemExit(main())
