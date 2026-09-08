#!/usr/bin/env python3

"""Summarise the weekly automated UI test results as a Slack message.

The results are the per-platform JUnit files that .github/workflows/weekly_ui_tests.yml
uploads as artifacts. This reads them and writes a Slack Block Kit payload to a file; it
does not post it. The workflow does that with curl, which keeps the webhook URL out of this
script's environment entirely.

The message is failure-list-first: every failing test is named, and no failure message or
traceback is included. It exists to answer "what broke, and is it the same thing as last
week" at a glance - the detail is one click away in the check runs and the test log
artifacts, and reproducing a failure needs the test name rather than its output. A test is
named by its JUnit `name` attribute alone, which for the ctest --output-junit file this
reads is the ctest test name, i.e. what `ctest -R <name>` takes.

A platform whose file is missing is reported rather than skipped. That is the case where
the build itself failed, which is both the most important thing to say and the easiest to
lose silently.
"""

import argparse
import json
import os
import pathlib
import sys
from xml.etree import ElementTree

# The matrix.os values of weekly_ui_tests.yml, which name its artifacts
PLATFORMS = ("Linux", "Windows")

# Where the workflow's download-artifact step puts them
ARTIFACT_DIR = pathlib.Path("artifacts")
ARTIFACT_NAME = "junit-automated-ui-{platform}"
JUNIT_FILENAME = "junit-uitest.xml"

# A section block's text field caps at 3000 characters, so a platform's failing tests are
# split over as many blocks as they need rather than being truncated. Kept below the limit
# so that adding the heading line to a chunk cannot push it over.
SECTION_TEXT_LIMIT = 2800

# A message caps at 50 blocks. Past this many the list is cut short with a count of the
# rest, so that a pathological run degrades instead of being rejected by the API.
MAX_BLOCKS = 40


def _summarise(junit_path: pathlib.Path) -> tuple[int, int, list]:
    """Return (total, skipped, failing test names) for one JUnit file.

    ctest --output-junit writes a single root <testsuite>; a <testsuites> wrapper is also
    accepted so that a file from any other producer reads the same way.
    """
    root = ElementTree.parse(junit_path).getroot()
    suites = [root] if root.tag == "testsuite" else root.iter("testsuite")

    total = skipped = 0
    failing = []
    for suite in suites:
        for case in suite.iter("testcase"):
            total += 1
            # ctest marks a test it did not run with status="notrun" as well as <skipped/>
            if case.find("skipped") is not None or case.get("status") == "notrun":
                skipped += 1
            elif case.find("failure") is not None or case.find("error") is not None:
                failing.append(case.get("name", "<unnamed test>"))

    return total, skipped, failing


def _failure_lines(names: list) -> list:
    """Return the bulleted failing test names, cut short if there are absurdly many."""
    listed = names[: MAX_BLOCKS * 10]
    lines = [f"    • `{name}`" for name in listed]
    if len(names) > len(listed):
        lines.append(f"    • …and {len(names) - len(listed)} more, see the run")

    return lines


def _chunk(heading: str, lines: list) -> list:
    """Return the section block texts for a heading and its lines, none over the size cap."""
    texts = []
    current = heading
    for line in lines:
        if len(current) + len(line) + 1 > SECTION_TEXT_LIMIT:
            texts.append(current)
            current = line
        else:
            current = f"{current}\n{line}"
    texts.append(current)

    return texts


def _platform_sections(platform: str) -> tuple[list, bool]:
    """Return the section block texts for one platform, and whether it needs attention."""
    junit_path = ARTIFACT_DIR / ARTIFACT_NAME.format(platform=platform) / JUNIT_FILENAME
    if not junit_path.exists():
        return [f":question: *{platform}* — no results (build or upload failed)"], True

    total, skipped, failing = _summarise(junit_path)
    if not failing:
        return [f":large_green_circle: *{platform}* — {total} passed, {skipped} skipped"], False

    heading = f":red_circle: *{platform}* — {len(failing)} of {total} failed, {skipped} skipped"

    return _chunk(heading, _failure_lines(failing)), True


def _run_url() -> str:
    return "{}/{}/actions/runs/{}".format(
        os.environ.get("GITHUB_SERVER_URL", "https://github.com"),
        os.environ.get("GITHUB_REPOSITORY", "mantidproject/mantid"),
        os.environ.get("GITHUB_RUN_ID", "0"),
    )


def _payload() -> dict:
    texts = []
    needs_attention = False
    for platform in PLATFORMS:
        platform_texts, platform_needs_attention = _platform_sections(platform)
        texts.extend(platform_texts)
        needs_attention = needs_attention or platform_needs_attention

    header = ":warning: Weekly UI tests: failures" if needs_attention else ":white_check_mark: Weekly UI tests: all green"

    # Two of the block budget are spent on the header and the context blocks
    sections = [{"type": "section", "text": {"type": "mrkdwn", "text": text}} for text in texts[: MAX_BLOCKS - 2]]

    return {
        # The notification preview and screen reader fallback: a Block Kit message without
        # it arrives looking empty
        "text": header,
        "blocks": [
            {"type": "header", "text": {"type": "plain_text", "text": header, "emoji": True}},
            *sections,
            {"type": "context", "elements": [{"type": "mrkdwn", "text": f"<{_run_url()}|Full run and artifacts>"}]},
        ],
    }


def _parse_args(argv: list) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarise the weekly automated UI test results as a Slack message.")
    parser.add_argument("output_file", type=pathlib.Path, help="File to write the Slack Block Kit payload to")

    return parser.parse_args(argv)


def main() -> int:
    args = _parse_args(sys.argv[1:])
    payload = _payload()
    args.output_file.write_text(json.dumps(payload), encoding="utf-8")
    print(f"Wrote Slack payload to {args.output_file}:")
    print(json.dumps(payload, indent=2))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
