#!/usr/bin/env python3

"""Unit tests for count_warnings.

These cover the log parsing and the exit conditions, which is everything that can
regress without a compiler or a GitHub Actions runner. Run them with:

    python -m unittest discover -s .github/actions/check-build-warnings

It deliberately omits a module-level test runner entry point, as it is not one of the
CMake-registered pytest suites. The check_build_warnings_tests.yml workflow runs it.

Each test is laid out as three blocks: write the build log, scan it, assert on the result.
"""

import contextlib
import io
import os
import pathlib
import tempfile
import unittest
from unittest import mock

import count_warnings

WARNING = "conversion from 'int' to 'char' may change value [-Wconversion]"


class CountWarningsTestCase(unittest.TestCase):
    def setUp(self):
        self._workspace_dir = tempfile.TemporaryDirectory()
        # Resolve so comparisons match count_warnings, which resolves both sides
        self.workspace = pathlib.Path(self._workspace_dir.name).resolve()
        self.build_dir = self.workspace / "build"
        self.addCleanup(self._workspace_dir.cleanup)

    def _write_log(self, *lines) -> pathlib.Path:
        """Write the given lines to a build log and return its path."""
        log_path = self.workspace / "build.log"
        log_path.write_text("\n".join(lines) + "\n")

        return log_path

    def _scan(self, log_path: pathlib.Path):
        """Scan a build log, returning (counted, uncounted, lines read)."""
        return count_warnings._scan(log_path, self.build_dir, self.workspace)

    def _run_main(self, argv, env=None):
        """Return (exit code, stdout) for a main() call, keeping the test output clean."""
        environment = {"GITHUB_WORKSPACE": str(self.workspace)}
        environment.update(env or {})
        stdout = io.StringIO()
        with mock.patch.object(count_warnings.sys, "argv", ["count_warnings.py", *argv]):
            with mock.patch.dict(os.environ, environment, clear=False):
                with contextlib.redirect_stdout(stdout):
                    exit_code = count_warnings.main()

        return exit_code, stdout.getvalue()

    def _build_dir_args(self) -> list:
        """The flags every main() call needs so relative log paths resolve like they do in CI."""
        return ["--build-dir", str(self.build_dir)]


class TestScan(CountWarningsTestCase):
    def test_counts_warning_for_workspace_source_file(self):
        log_path = self._write_log(f"{self.workspace}/Framework/Kernel/src/Foo.cpp:12:5: warning: {WARNING}")

        counted, uncounted, _ = self._scan(log_path)

        self.assertEqual(counted, [("Framework/Kernel/src/Foo.cpp", 12, 5, WARNING)])
        self.assertEqual(uncounted, [])

    def test_resolves_relative_paths_against_the_build_directory(self):
        # ninja emits paths relative to the build directory it runs in
        log_path = self._write_log(f"../Framework/Kernel/src/Foo.cpp:12:5: warning: {WARNING}")

        counted, _, _ = self._scan(log_path)

        self.assertEqual([warning[0] for warning in counted], ["Framework/Kernel/src/Foo.cpp"])

    def test_counts_warning_that_has_no_column(self):
        log_path = self._write_log(f"{self.workspace}/Framework/Kernel/src/Foo.cpp:12: warning: {WARNING}")

        counted, _, _ = self._scan(log_path)

        self.assertEqual(counted, [("Framework/Kernel/src/Foo.cpp", 12, 0, WARNING)])

    def test_deduplicates_the_same_warning_reported_by_several_translation_units(self):
        header_warning = f"{self.workspace}/Framework/Kernel/inc/MantidKernel/Foo.h:8:3: warning: {WARNING}"
        log_path = self._write_log(header_warning, "[2/9] Building CXX object Bar.cpp.o", header_warning)

        counted, _, _ = self._scan(log_path)

        self.assertEqual(len(counted), 1)

    def test_counts_warnings_that_differ_only_by_line(self):
        log_path = self._write_log(
            f"{self.workspace}/Framework/Kernel/src/Foo.cpp:12:5: warning: {WARNING}",
            f"{self.workspace}/Framework/Kernel/src/Foo.cpp:13:5: warning: {WARNING}",
        )

        counted, _, _ = self._scan(log_path)

        self.assertEqual(len(counted), 2)

    def test_excludes_generated_and_third_party_paths(self):
        generated_and_third_party = (
            f"{self.build_dir}/_deps/googletest-src/gtest.h:10:1: warning: {WARNING}",
            f"{self.build_dir}/qt/generated/moc_Foo.cpp:10:1: warning: {WARNING}",
            f"{self.workspace}/_deps/eigen-src/Eigen/Core.h:44:9: warning: {WARNING}",
            f"{self.workspace}/.pixi/envs/default/include/thing.hpp:99:1: warning: {WARNING}",
        )
        log_path = self._write_log(*generated_and_third_party)

        counted, uncounted, _ = self._scan(log_path)

        self.assertEqual(counted, [])
        self.assertEqual(len(uncounted), len(generated_and_third_party))

    def test_excludes_paths_outside_the_workspace(self):
        log_path = self._write_log(f"/opt/conda/envs/mantid/include/boost/thing.hpp:99:1: warning: {WARNING}")

        counted, uncounted, _ = self._scan(log_path)

        self.assertEqual(counted, [])
        self.assertEqual(len(uncounted), 1)

    def test_excludes_files_that_are_not_c_or_cpp(self):
        log_path = self._write_log(f"{self.workspace}/docs/source/algorithms/Foo.py:1:1: warning: {WARNING}")

        counted, uncounted, _ = self._scan(log_path)

        self.assertEqual(counted, [])
        self.assertEqual(len(uncounted), 1)

    def test_records_warnings_without_a_source_location_as_uncounted(self):
        log_path = self._write_log(
            "cc1plus: warning: command-line option '-Wfoo' is valid for C but not for C++",
            "/usr/bin/ld: warning: libfoo.so, needed by libbar.so, not found",
        )

        counted, uncounted, _ = self._scan(log_path)

        self.assertEqual(counted, [])
        self.assertEqual(len(uncounted), 2)

    def test_ignores_lines_that_are_not_warnings(self):
        log_path = self._write_log(
            "[1/9] Building CXX object Framework/Kernel/CMakeFiles/Kernel.dir/src/Foo.cpp.o",
            f"{self.workspace}/Framework/Kernel/src/Foo.cpp: In function 'char narrow(int)':",
            f"{self.workspace}/Framework/Kernel/src/Foo.cpp:12:5: note: in expansion of macro 'FOO'",
            "CMake Warning at CMakeLists.txt:1 (message):",
            f"{self.workspace}/Framework/Kernel/src/Foo.cpp:12:5: error: something broke",
        )

        counted, uncounted, lines_read = self._scan(log_path)

        self.assertEqual(counted, [])
        self.assertEqual(uncounted, [])
        self.assertEqual(lines_read, 5)

    def test_strips_ansi_escape_sequences(self):
        coloured_warning = (
            f"\x1b[01m\x1b[K{self.workspace}/Framework/Kernel/src/Foo.cpp:7:3:\x1b[m\x1b[K "
            "\x1b[01;35m\x1b[Kwarning: \x1b[m\x1b[Kunused variable 'x' [\x1b[01;35m\x1b[K-Wunused-variable\x1b[m\x1b[K]"
        )
        log_path = self._write_log(coloured_warning)

        counted, _, _ = self._scan(log_path)

        self.assertEqual(counted, [("Framework/Kernel/src/Foo.cpp", 7, 3, "unused variable 'x' [-Wunused-variable]")])

    def test_counts_lines_read(self):
        log_path = self._write_log("one", "two", "three")

        _, _, lines_read = self._scan(log_path)

        self.assertEqual(lines_read, 3)


class TestWarningFlag(unittest.TestCase):
    def test_extracts_the_trailing_warning_flag(self):
        message = WARNING

        flag = count_warnings._warning_flag(message)

        self.assertEqual(flag, "-Wconversion")

    def test_reports_a_warning_with_no_flag_as_unflagged(self):
        message = "something happened"

        flag = count_warnings._warning_flag(message)

        self.assertEqual(flag, "unflagged")


class TestMain(CountWarningsTestCase):
    def test_fails_when_the_log_is_missing(self):
        missing_log = self.workspace / "absent.log"

        exit_code, output = self._run_main([str(missing_log)])

        self.assertEqual(exit_code, 1)
        self.assertIn("does not exist", output)

    def test_fails_when_the_log_is_empty(self):
        empty_log = self.workspace / "empty.log"
        empty_log.touch()

        exit_code, output = self._run_main([str(empty_log)])

        self.assertEqual(exit_code, 1)
        self.assertIn("is empty", output)

    def test_passes_when_there_are_no_countable_warnings(self):
        log_path = self._write_log(
            "[1/1] Building CXX object Foo.cpp.o",
            "cc1plus: warning: some toolchain noise",
        )

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args()])

        self.assertEqual(exit_code, 0)
        self.assertIn("No compiler warnings found", output)

    def test_fails_when_there_are_countable_warnings(self):
        log_path = self._write_log(f"../Framework/Kernel/src/Foo.cpp:12:5: warning: {WARNING}")

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args()])

        self.assertEqual(exit_code, 1)
        self.assertIn("::error::Build produced 1 compiler warning(s)", output)
        self.assertIn("Framework/Kernel/src/Foo.cpp:12:5", output)
        self.assertIn("-Wconversion", output)

    def test_writes_the_warnings_to_the_step_summary(self):
        log_path = self._write_log(f"../Framework/Kernel/src/Foo.cpp:12:5: warning: {WARNING}")
        summary_path = self.workspace / "summary.md"

        self._run_main([str(log_path), *self._build_dir_args()], env={"GITHUB_STEP_SUMMARY": str(summary_path)})

        summary = summary_path.read_text()
        self.assertIn("1 compiler warning(s)", summary)
        self.assertIn("Framework/Kernel/src/Foo.cpp:12:5", summary)


class TestAllowedWarnings(CountWarningsTestCase):
    def _log_with_warnings(self, count: int) -> pathlib.Path:
        lines = [f"../Framework/Kernel/src/Foo.cpp:{line}:5: warning: {WARNING}" for line in range(1, count + 1)]

        return self._write_log(*lines)

    def test_defaults_to_allowing_no_warnings(self):
        log_path = self._log_with_warnings(1)

        exit_code, _ = self._run_main([str(log_path), *self._build_dir_args()])

        self.assertEqual(exit_code, 1)

    def test_passes_when_the_count_is_within_the_allowance(self):
        log_path = self._log_with_warnings(3)

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args(), "--allowed-warnings", "3"])

        self.assertEqual(exit_code, 0)
        self.assertIn("::warning::Build produced 3 compiler warning(s), within the allowance of 3", output)

    def test_passes_when_the_count_is_below_the_allowance(self):
        log_path = self._log_with_warnings(1)

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args(), "--allowed-warnings", "5"])

        self.assertEqual(exit_code, 0)
        self.assertIn("within the allowance of 5", output)

    def test_fails_when_the_count_exceeds_the_allowance(self):
        log_path = self._log_with_warnings(4)

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args(), "--allowed-warnings", "3"])

        self.assertEqual(exit_code, 1)
        self.assertIn("::error::Build produced 4 compiler warning(s). Only 3 are allowed.", output)

    def test_reports_no_warnings_rather_than_an_allowance_when_the_build_is_clean(self):
        log_path = self._write_log("[1/1] Building CXX object Foo.cpp.o")

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args(), "--allowed-warnings", "3"])

        self.assertEqual(exit_code, 0)
        self.assertIn("No compiler warnings found", output)
        self.assertNotIn("allowance", output)

    def test_rejects_a_negative_allowance(self):
        log_path = self._log_with_warnings(1)

        exit_code, output = self._run_main([str(log_path), *self._build_dir_args(), "--allowed-warnings", "-1"])

        self.assertEqual(exit_code, 1)
        self.assertIn("must not be negative", output)
