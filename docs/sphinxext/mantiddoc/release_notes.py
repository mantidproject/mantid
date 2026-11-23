# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Order the release notes in the navigation sidebar newest first.

``release/index.rst`` pulls in every release with a globbing toctree entry (``*/index``). Sphinx sorts glob matches
alphabetically, so the sidebar lists the oldest release first and puts e.g. v6.10.0 before v6.2.0. Just before the page
is parsed, this extension swaps the glob entry for an explicit entry per version directory, sorted by version number
with the newest first. New release directories are picked up automatically, so nothing needs editing at release time.
"""

import re
from pathlib import Path

from sphinx.util import logging

logger = logging.getLogger(__name__)

RELEASE_INDEX_DOCNAME = "release/index"
VERSION_DIR_RE = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")
GLOB_ENTRY_RE = re.compile(r"^(?P<indent>[ \t]+)\*/index[ \t]*$", re.MULTILINE)


def find_release_versions(release_dir: Path) -> list[str]:
    """Names of the ``vX.Y.Z`` directories below ``release_dir`` that contain an index page, newest first."""
    versions = []
    for index in release_dir.glob("*/index.rst"):
        match = VERSION_DIR_RE.match(index.parent.name)
        if match:
            versions.append((tuple(int(part) for part in match.groups()), index.parent.name))
    return [name for _, name in sorted(versions, reverse=True)]


def order_release_toctree(source: str, versions: list[str]) -> str:
    """Replace the ``*/index`` toctree entry in ``source`` with one entry per version, in the order given."""
    match = GLOB_ENTRY_RE.search(source)
    if match is None or not versions:
        return source
    entries = "\n".join(f"{match.group('indent')}{version}/index" for version in versions)
    return source[: match.start()] + entries + source[match.end() :]


def _on_source_read(app, docname, source):
    if docname != RELEASE_INDEX_DOCNAME:
        return
    versions = find_release_versions(Path(app.srcdir) / "release")
    ordered = order_release_toctree(source[0], versions)
    if ordered == source[0]:
        logger.warning(f"Could not order the release notes toctree in {docname}: no '*/index' entry or no releases found")
    source[0] = ordered


def _on_env_get_outdated(app, env, added, changed, removed):
    # Once the glob is replaced Sphinx no longer knows this page depends on the release directories,
    # so re-read it in incremental builds whenever a release index page appears or disappears.
    if any(VERSION_DIR_RE.match(docname.split("/")[1]) for docname in (added | removed) if docname.startswith("release/v")):
        return [RELEASE_INDEX_DOCNAME]
    return []


def setup(app):
    app.connect("source-read", _on_source_read)
    app.connect("env-get-outdated", _on_env_get_outdated)
    return {"parallel_read_safe": True, "parallel_write_safe": True}
