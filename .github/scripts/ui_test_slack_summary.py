#!/usr/bin/env python3

"""Summarise the weekly automated UI test results as a Slack message.

Reads the per-platform JUnit files that .github/workflows/weekly_ui_tests.yml uploads as
artifacts and writes a Slack Block Kit payload. The workflow posts it with curl, so the
webhook URL never enters this script's environment.

Every failing test is named and no failure output is included: the JUnit name is the ctest
test name, i.e. what `ctest -R` takes to reproduce one, and the output is a click away in
the check runs and the test log artifacts. A platform whose file is missing is reported
rather than skipped - that is the run where the build itself died.
"""

import argparse
import json
import os
import pathlib
import sys
from xml.etree import ElementTree

PLATFORMS = ("Linux", "Windows")

# Where weekly_ui_tests.yml's download-artifact step puts each platform's results
JUNIT_PATH = "artifacts/junit-automated-ui-{platform}/junit-uitest.xml"

# A section block's text caps at 3000 characters, so a long list is split over several
SECTION_TEXT_LIMIT = 2800

# The suite is ~25 tests, so this never truncates in practice. It is here to bound the
# block count, as a message caps at 50 blocks.
MAX_LISTED_FAILURES = 100


def _summarise(junit_path: pathlib.Path) -> tuple[int, int, list]:
    """Return (total, skipped, failing test names) for one JUnit file."""
    root = ElementTree.parse(junit_path).getroot()
    # ctest --output-junit writes a single root <testsuite>; a <testsuites> wrapper is accepted too
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


def _failure_sections(heading: str, names: list) -> list:
    """Return the section texts listing the failing tests, none of them over the size cap."""
    lines = [f"    • `{name}`" for name in names[:MAX_LISTED_FAILURES]]
    if len(names) > MAX_LISTED_FAILURES:
        lines.append(f"    • …and {len(names) - MAX_LISTED_FAILURES} more, see the run")

    texts = [heading]
    for line in lines:
        if len(texts[-1]) + len(line) + 1 > SECTION_TEXT_LIMIT:
            texts.append(line)
        else:
            texts[-1] = f"{texts[-1]}\n{line}"

    return texts


def _platform_sections(platform: str) -> tuple[list, bool]:
    """Return the section texts for one platform, and whether it needs attention."""
    junit_path = pathlib.Path(JUNIT_PATH.format(platform=platform))
    if not junit_path.exists():
        return [f":question: *{platform}* — no results (build or upload failed)"], True

    total, skipped, failing = _summarise(junit_path)
    if not failing:
        return [f":large_green_circle: *{platform}* — {total} passed, {skipped} skipped"], False

    return _failure_sections(f":red_circle: *{platform}* — {len(failing)} of {total} failed, {skipped} skipped", failing), True


def _context() -> str:
    """The branch and run link. The branch is named because a dispatch otherwise looks like the Sunday run."""
    env = os.environ.get
    branch = env("GITHUB_REF_NAME", "unknown").replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    repository = f"{env('GITHUB_SERVER_URL', 'https://github.com')}/{env('GITHUB_REPOSITORY', 'mantidproject/mantid')}"

    return f"Branch `{branch}` · <{repository}/actions/runs/{env('GITHUB_RUN_ID', '0')}|Full run and artifacts>"


def _payload() -> dict:
    texts = []
    needs_attention = False
    for platform in PLATFORMS:
        platform_texts, platform_failed = _platform_sections(platform)
        texts.extend(platform_texts)
        needs_attention |= platform_failed

    header = ":warning: Weekly UI tests: failures" if needs_attention else ":white_check_mark: Weekly UI tests: all green"

    return {
        # The notification preview and screen reader fallback; without it the message arrives looking empty
        "text": header,
        "blocks": [
            {"type": "header", "text": {"type": "plain_text", "text": header, "emoji": True}},
            *({"type": "section", "text": {"type": "mrkdwn", "text": text}} for text in texts),
            {"type": "context", "elements": [{"type": "mrkdwn", "text": _context()}]},
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarise the weekly automated UI test results as a Slack message.")
    parser.add_argument("output_file", type=pathlib.Path, help="File to write the Slack Block Kit payload to")
    args = parser.parse_args(sys.argv[1:])

    payload = _payload()
    args.output_file.write_text(json.dumps(payload), encoding="utf-8")
    print(json.dumps(payload, indent=2))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
