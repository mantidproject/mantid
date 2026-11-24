# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Tests for ordering the release notes toctree
"""

import tempfile
import unittest
from pathlib import Path

from mantiddoc.release_notes import find_release_versions, order_release_toctree

INDEX_SOURCE = """Release Notes
=============

.. toctree::
   :maxdepth: 1
   :hidden:
   :glob:

   */index

* v6.10.0 - :doc:`release notes <v6.10.0/index>`
"""


class FindReleaseVersionsTest(unittest.TestCase):
    def test_versions_are_sorted_numerically_newest_first(self):
        with tempfile.TemporaryDirectory() as tmp:
            release_dir = Path(tmp)
            for name in ["v6.2.0", "v6.10.0", "v6.9.1", "v7.0.0", "v3.13.0", "v6.16.1"]:
                (release_dir / name).mkdir()
                (release_dir / name / "index.rst").touch()

            self.assertEqual(["v7.0.0", "v6.16.1", "v6.10.0", "v6.9.1", "v6.2.0", "v3.13.0"], find_release_versions(release_dir))

    def test_index_pages_in_markdown_are_found(self):
        with tempfile.TemporaryDirectory() as tmp:
            release_dir = Path(tmp)
            (release_dir / "v3.5.1").mkdir()
            (release_dir / "v3.5.1" / "index.md").touch()
            (release_dir / "v7.0.0").mkdir()
            (release_dir / "v7.0.0" / "index.rst").touch()

            self.assertEqual(["v7.0.0", "v3.5.1"], find_release_versions(release_dir))

    def test_directories_that_are_not_releases_are_ignored(self):
        with tempfile.TemporaryDirectory() as tmp:
            release_dir = Path(tmp)
            (release_dir / "templates").mkdir()
            (release_dir / "templates" / "index.rst").touch()
            (release_dir / "v7.1.0").mkdir()
            (release_dir / "v7.1.0" / "framework.rst").touch()
            (release_dir / "v7.0.0").mkdir()
            (release_dir / "v7.0.0" / "index.rst").touch()

            self.assertEqual(["v7.0.0"], find_release_versions(release_dir))


class OrderReleaseToctreeTest(unittest.TestCase):
    def test_glob_entry_is_replaced_with_ordered_entries(self):
        ordered = order_release_toctree(INDEX_SOURCE, ["v6.10.0", "v6.2.0"])

        self.assertIn("   :glob:\n\n   v6.10.0/index\n   v6.2.0/index\n\n* v6.10.0", ordered)
        self.assertNotIn("*/index", ordered)

    def test_source_is_unchanged_without_glob_entry(self):
        source = INDEX_SOURCE.replace("   */index\n", "")

        self.assertEqual(source, order_release_toctree(source, ["v6.10.0"]))

    def test_source_is_unchanged_without_versions(self):
        self.assertEqual(INDEX_SOURCE, order_release_toctree(INDEX_SOURCE, []))


if __name__ == "__main__":
    unittest.main()
