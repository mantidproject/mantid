# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Check that every release note is reachable from an ``.. amalgamate::`` directive.

Individual release notes are not pulled into the documentation by a toctree, they are read by the
``amalgamate`` directive (docs/sphinxext/mantiddoc/directives/amalgamate.py) from the directory named as
its argument. Notes sitting in a directory that no page names are therefore silently dropped from the
published release notes, and conf.py deliberately excludes them from Sphinx's "document isn't included in
any toctree" warning. This script closes that gap by checking both directions:

- a directory of notes that no ``.. amalgamate::`` directive points at, and
- an ``.. amalgamate::`` directive pointing at a directory that does not exist (the directive silently
  does nothing in that case).

Releases that have already been published are skipped: release_editor.py replaces the directives with the
collated note text, so a version directory with no directives left has nothing to check.
"""

import sys
from argparse import ArgumentParser
from pathlib import Path
import re

# Matches the directive as written in the release pages, e.g. ".. amalgamate:: Diffraction/Powder/Bugfixes"
AMALGAMATE_RE = re.compile(r"^\s*\.\.\s+amalgamate::\s+(\S+)\s*$")

# release_editor.py moves notes here once they have been collated into the top level page
PUBLISHED_DIR_NAME = "Used"

REPO_ROOT = Path(__file__).resolve().parents[2]
RELEASE_ROOT = REPO_ROOT / "docs" / "source" / "release"


def display_path(path: Path) -> str:
    """Path relative to the repository root, using forward slashes on every platform.

    Anything outside the repository is reported in full, which keeps the checker usable against the
    throwaway release trees built by the tests.
    """
    try:
        return path.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def find_amalgamate_targets(version_dir: Path) -> dict[Path, list[tuple[Path, int]]]:
    """Map each directory named by an ``.. amalgamate::`` directive to the pages and lines referencing it."""
    targets: dict[Path, list[tuple[Path, int]]] = {}
    for page in sorted(version_dir.glob("*.rst")):
        for line_number, line in enumerate(page.read_text(encoding="utf-8").splitlines(), start=1):
            match = AMALGAMATE_RE.match(line)
            if match is None:
                continue
            # the directive treats a leading slash as relative to the version directory too
            target = version_dir.joinpath(*match.group(1).strip("/").split("/"))
            targets.setdefault(target, []).append((page, line_number))
    return targets


def find_note_directories(version_dir: Path) -> set[Path]:
    """All directories below ``version_dir`` holding notes that still need to be picked up by a directive."""
    directories = set()
    for note in version_dir.rglob("*.rst"):
        parent = note.parent
        if parent == version_dir:
            # the top level pages are reached through the index toctree, not through a directive
            continue
        if PUBLISHED_DIR_NAME in parent.relative_to(version_dir).parts:
            continue
        directories.add(parent)
    return directories


def check_version(version_dir: Path) -> list[str]:
    """Return a problem description for every unreferenced note directory and every dangling directive."""
    targets = find_amalgamate_targets(version_dir)
    if not targets:
        # already released, the directives have been replaced by the collated notes
        return []

    problems = []
    for directory in sorted(find_note_directories(version_dir) - set(targets)):
        notes = sorted(note.name for note in directory.glob("*.rst"))
        plural = "note" if len(notes) == 1 else "notes"
        problems.append(
            f"{display_path(directory)} holds {len(notes)} release {plural} that no page references:\n"
            + "".join(f"    {name}\n" for name in notes)
            + f"  These notes will be missing from the published release notes. Add\n"
            f"    .. amalgamate:: {directory.relative_to(version_dir).as_posix()}\n"
            f"  under a heading in the relevant page in {display_path(version_dir)}/."
        )

    for target, references in sorted(targets.items()):
        if target.is_dir():
            continue
        for page, line_number in references:
            problems.append(
                f"{display_path(page)}:{line_number}: amalgamate directive points at "
                f"{display_path(target)}, which does not exist. Any notes intended for it will be missing "
                f"from the published release notes."
            )

    return problems


def find_version_directories(release: str | None) -> list[Path]:
    if release is not None:
        if not release.startswith("v"):
            release = "v" + release
        return [RELEASE_ROOT / release]
    return sorted(path for path in RELEASE_ROOT.glob("v*") if path.is_dir())


def main() -> int:
    parser = ArgumentParser(description="Find release notes that no amalgamate directive references")
    parser.add_argument("--release", help="only check this release, e.g. v7.0.0. Defaults to every release.")
    args = parser.parse_args()

    problems = []
    for version_dir in find_version_directories(args.release):
        if not version_dir.is_dir():
            print(f"No such release directory: {display_path(version_dir)}", file=sys.stderr)
            return 1
        problems += check_version(version_dir)

    for problem in problems:
        print(problem)

    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
