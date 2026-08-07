# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import io
import sys
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import mock

import unreferenced_release_note_check as checker


class UnreferencedReleaseNoteCheckTest(unittest.TestCase):
    def setUp(self):
        temp_dir = TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        self.release_root = Path(temp_dir.name)

    # ------------------------------------------------------------------ helpers

    def _make_version(self, name: str = "v1.0.0") -> Path:
        version_dir = self.release_root / name
        version_dir.mkdir()
        return version_dir

    def _write_page(self, version_dir: Path, name: str, *targets: str) -> Path:
        """Write a top level release page referencing zero or more note directories."""
        directives = "".join(f"\n.. amalgamate:: {target}\n" for target in targets)
        page = version_dir / name
        page.write_text(f"=====\nTitle\n=====\n{directives}", encoding="utf-8")
        return page

    def _write_note(self, version_dir: Path, relative_dir: str, name: str = "12345.rst") -> Path:
        note_dir = version_dir / relative_dir
        note_dir.mkdir(parents=True, exist_ok=True)
        note = note_dir / name
        note.write_text("- A release note.\n", encoding="utf-8")
        return note

    def _run_main(self, argv: list) -> tuple:
        """Run main() against the temporary release tree, returning (exit code, stdout, stderr)."""
        stdout, stderr = io.StringIO(), io.StringIO()
        with (
            mock.patch.object(checker, "RELEASE_ROOT", self.release_root),
            mock.patch.object(sys, "argv", ["unreferenced_release_note_check.py"] + argv),
            redirect_stdout(stdout),
            redirect_stderr(stderr),
        ):
            exit_code = checker.main()
        return exit_code, stdout.getvalue(), stderr.getvalue()

    # ------------------------------------------------------------------ no problems

    def test_release_with_every_directory_referenced_has_no_problems(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/New_features")
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features", "Framework/Python/Bugfixes")

        self.assertEqual([], checker.check_version(version_dir))

    def test_top_level_pages_are_not_reported_as_unreferenced(self):
        # the version index toctree picks these up, they are not read through a directive
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst", "Framework/Python/Bugfixes")
        self._write_page(version_dir, "index.rst")

        self.assertEqual([], checker.check_version(version_dir))

    def test_directory_holding_no_notes_is_not_reported(self):
        # release.py creates the full directory tree up front and marks empty directories with .gitkeep
        version_dir = self._make_version()
        empty_dir = version_dir / "Framework" / "Python" / "Bugfixes"
        empty_dir.mkdir(parents=True)
        (empty_dir / ".gitkeep").touch()
        self._write_note(version_dir, "Framework/Python/New_features")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features")

        self.assertEqual([], checker.check_version(version_dir))

    def test_notes_already_moved_to_used_are_ignored(self):
        # release_editor.py moves notes into Used/ once they have been collated into the page
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes/Used")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features")
        self._write_note(version_dir, "Framework/Python/New_features")

        self.assertEqual([], checker.check_version(version_dir))

    def test_released_version_with_no_directives_is_skipped(self):
        # release_editor.py replaces the directives with the collated text, so there is nothing left to check
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst")

        self.assertEqual([], checker.check_version(version_dir))

    # ------------------------------------------------------------------ unreferenced notes

    def test_unreferenced_directory_is_reported(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/New_features")
        self._write_note(version_dir, "Framework/Python/Deprecated", name="41756.rst")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features")

        problems = checker.check_version(version_dir)

        self.assertEqual(1, len(problems))
        self.assertIn("Framework/Python/Deprecated", problems[0])
        self.assertIn("41756.rst", problems[0])

    def test_report_names_the_directive_that_should_be_added(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/New_features")
        self._write_note(version_dir, "Framework/Python/Deprecated")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features")

        problems = checker.check_version(version_dir)

        self.assertIn(".. amalgamate:: Framework/Python/Deprecated", problems[0])

    def test_unreferenced_directory_nested_below_a_new_sub_topic_is_reported(self):
        # the case that motivated the check: a new sub topic reusing a standard category name
        version_dir = self._make_version()
        self._write_note(version_dir, "Diffraction/Powder/New_features")
        self._write_note(version_dir, "Diffraction/TotalScattering/New_features", name="41234.rst")
        self._write_page(version_dir, "diffraction.rst", "Diffraction/Powder/New_features")

        problems = checker.check_version(version_dir)

        self.assertEqual(1, len(problems))
        self.assertIn(".. amalgamate:: Diffraction/TotalScattering/New_features", problems[0])

    def test_every_unreferenced_directory_is_reported(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/New_features")
        self._write_note(version_dir, "Framework/Python/Deprecated")
        self._write_note(version_dir, "Framework/Python/Removed")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features")

        problems = checker.check_version(version_dir)

        self.assertEqual(2, len(problems))

    def test_note_count_is_pluralised(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/New_features")
        self._write_page(version_dir, "framework.rst", "Framework/Python/Bugfixes")
        self._write_note(version_dir, "Framework/Python/Bugfixes")

        single = checker.check_version(version_dir)
        self.assertIn("1 release note that", single[0])

        self._write_note(version_dir, "Framework/Python/New_features", name="99999.rst")
        plural = checker.check_version(version_dir)
        self.assertIn("2 release notes that", plural[0])

    # ------------------------------------------------------------------ directive targets

    def test_directive_pointing_at_a_missing_directory_is_reported_with_page_and_line(self):
        version_dir = self._make_version()
        page = version_dir / "framework.rst"
        # the directive is deliberately on the fifth line
        page.write_text("=====\nTitle\n=====\n\n.. amalgamate:: Framework/Typo/Bugfixes\n", encoding="utf-8")

        problems = checker.check_version(version_dir)

        self.assertEqual(1, len(problems))
        self.assertIn(f"{page.name}:5:", problems[0])
        self.assertIn("does not exist", problems[0])

    def test_missing_directory_is_reported_once_per_referencing_line(self):
        version_dir = self._make_version()
        self._write_page(version_dir, "framework.rst", "Framework/Typo/Bugfixes")
        self._write_page(version_dir, "diffraction.rst", "Framework/Typo/Bugfixes")

        problems = checker.check_version(version_dir)

        self.assertEqual(2, len(problems))

    def test_leading_slash_in_directive_resolves_against_the_version_directory(self):
        # amalgamate.py prepends a slash before joining, so both spellings mean the same directory
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst", "/Framework/Python/Bugfixes")

        self.assertEqual([], checker.check_version(version_dir))

    def test_indented_directive_is_recognised(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        (version_dir / "framework.rst").write_text(
            "=====\nTitle\n=====\n\n   .. amalgamate:: Framework/Python/Bugfixes\n", encoding="utf-8"
        )

        self.assertEqual([], checker.check_version(version_dir))

    def test_directories_may_be_referenced_from_more_than_one_page(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst", "Framework/Python/Bugfixes")
        self._write_page(version_dir, "diffraction.rst", "Framework/Python/Bugfixes")

        self.assertEqual([], checker.check_version(version_dir))

    # ------------------------------------------------------------------ command line

    def test_main_returns_zero_when_every_release_is_clean(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst", "Framework/Python/Bugfixes")

        exit_code, stdout, _ = self._run_main([])

        self.assertEqual(0, exit_code)
        self.assertEqual("", stdout)

    def test_main_returns_one_and_reports_when_a_release_has_problems(self):
        version_dir = self._make_version()
        self._write_note(version_dir, "Framework/Python/Deprecated")
        self._write_page(version_dir, "framework.rst", "Framework/Python/New_features")
        self._write_note(version_dir, "Framework/Python/New_features")

        exit_code, stdout, _ = self._run_main([])

        self.assertEqual(1, exit_code)
        self.assertIn("Framework/Python/Deprecated", stdout)

    def test_main_checks_every_release_directory_by_default(self):
        clean = self._make_version("v1.0.0")
        self._write_note(clean, "Framework/Python/Bugfixes")
        self._write_page(clean, "framework.rst", "Framework/Python/Bugfixes")
        broken = self._make_version("v2.0.0")
        self._write_note(broken, "Framework/Python/Deprecated")
        self._write_page(broken, "framework.rst", "Framework/Python/New_features")
        self._write_note(broken, "Framework/Python/New_features")

        exit_code, stdout, _ = self._run_main([])

        self.assertEqual(1, exit_code)
        self.assertIn("v2.0.0", stdout)
        self.assertNotIn("v1.0.0", stdout)

    def test_main_can_be_limited_to_a_single_release(self):
        broken = self._make_version("v2.0.0")
        self._write_note(broken, "Framework/Python/Deprecated")
        self._write_page(broken, "framework.rst", "Framework/Python/New_features")
        self._write_note(broken, "Framework/Python/New_features")
        clean = self._make_version("v1.0.0")
        self._write_note(clean, "Framework/Python/Bugfixes")
        self._write_page(clean, "framework.rst", "Framework/Python/Bugfixes")

        self.assertEqual(0, self._run_main(["--release", "v1.0.0"])[0])
        self.assertEqual(1, self._run_main(["--release", "v2.0.0"])[0])

    def test_main_accepts_a_release_without_the_v_prefix(self):
        version_dir = self._make_version("v1.0.0")
        self._write_note(version_dir, "Framework/Python/Bugfixes")
        self._write_page(version_dir, "framework.rst", "Framework/Python/Bugfixes")

        self.assertEqual(0, self._run_main(["--release", "1.0.0"])[0])

    def test_main_returns_one_for_an_unknown_release(self):
        exit_code, _, stderr = self._run_main(["--release", "v9.9.9"])

        self.assertEqual(1, exit_code)
        self.assertIn("No such release directory", stderr)


if __name__ == "__main__":
    unittest.main()
