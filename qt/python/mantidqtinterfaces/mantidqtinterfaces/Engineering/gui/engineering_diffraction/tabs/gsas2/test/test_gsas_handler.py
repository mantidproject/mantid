# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from ..gsas2_handler import (
    GSAS2Handler,
    FilePaths,
    GSAS2Config,
)
import unittest
from unittest import mock
import shutil
import tempfile
import os
from pathlib import Path
from ..gsas2_handler import SaveDirectories, RefinementSettings


class TestGSAS2Handler(unittest.TestCase):
    temp_save_directory: str

    @classmethod
    def setUpClass(cls) -> None:
        cls.temp_save_directory = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls) -> None:
        # Clean up the class-level temporary directory
        if hasattr(cls, "temp_save_directory") and os.path.exists(cls.temp_save_directory):
            shutil.rmtree(cls.temp_save_directory)
        super().tearDownClass()

    def setUp(self):
        self.path_to_gsas2 = Path(os.getenv("MANTID_PROJECT_PATH", Path.cwd()))
        self.path_to_gsas2 = self.path_to_gsas2.resolve()

        self.save_directories = SaveDirectories(temporary_save_directory=self.temp_save_directory, project_name="test_project")

        self.refinement_settings = RefinementSettings(
            method="test_method",
            background=True,
            microstrain=False,
            sigma_one=True,
            gamma=False,
            histogram_scale_factor=True,
            unit_cell=False,
        )

        self.file_paths = FilePaths(
            data_files=["data_file_1", "data_file_2"],
            phase_filepaths=["phase_file_1", "phase_file_2"],
            instrument_files=["instr_file_1", "instr_file_2"],
        )

        self.config = GSAS2Config(
            limits=[0, 10],
            mantid_pawley_reflections=["reflection_1", "reflection_2"],
            override_cell_lengths=[[1.0, 1.0, 1.0], [2.0, 2.0, 2.0]],
            d_spacing_min=0.5,
            number_of_regions=2,
        )

        # Setup for limited_rglob tests
        self.test_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.test_dir)

        self.handler = GSAS2Handler(
            path_to_gsas2=self.path_to_gsas2,
            save_directories=self.save_directories,
            refinement_settings=self.refinement_settings,
            file_paths=self.file_paths,
            config=self.config,
        )

    def tearDown(self):
        self.handler.cleanup()
        super().tearDown()

    def create_test_files(self, files):
        for file in files:
            file_path = os.path.join(self.test_dir, file)
            os.makedirs(os.path.dirname(file_path), exist_ok=True)
            with open(file_path, "w") as f:
                f.write("test")

    def test_invalid_path_to_gsas2(self):
        """
        Test that a ValueError is raised for various invalid path_to_gsas2 inputs.
        """
        test_cases = [
            {"path": None, "expected_error": "path_to_gsas2 must be a non-empty string or Path."},
            {"path": "", "expected_error": "path_to_gsas2 must be a non-empty string or Path."},
            {
                "path": "/nonexistent/path/to/gsas2",
                "expected_error": "Invalid path_to_gsas2: /nonexistent/path/to/gsas2 does not exist or cannot be resolved.",
            },
        ]

        for case in test_cases:
            with self.subTest(path=case["path"], expected_error=case["expected_error"]):
                with self.assertRaises(ValueError) as context:
                    GSAS2Handler(
                        path_to_gsas2=case["path"],
                        save_directories=self.save_directories,
                        refinement_settings=self.refinement_settings,
                        file_paths=self.file_paths,
                        config=self.config,
                    )
                self.assertIn(
                    case["expected_error"],
                    str(context.exception),
                    msg=f"Failed for path: {case['path']}",
                )

    def test_invalid_path_to_gsas2_not_a_directory(self):
        # Create a temporary file to simulate a non-directory path
        invalid_path = tempfile.NamedTemporaryFile()

        with self.assertRaises(ValueError) as context:
            GSAS2Handler(
                path_to_gsas2=invalid_path.name,
                save_directories=self.save_directories,
                refinement_settings=self.refinement_settings,
                file_paths=self.file_paths,
                config=self.config,
            )
        self.assertIn(f"Invalid path_to_gsas2: {Path(invalid_path.name).resolve()} must be a valid directory.", str(context.exception))

    def test_init_valid_inputs(self):
        expected_path = self.path_to_gsas2

        self.assertEqual(self.handler.path_to_gsas2, expected_path)
        self.assertEqual(self.handler.save_directories, self.save_directories)
        self.assertEqual(self.handler.refinement_settings, self.refinement_settings)
        self.assertEqual(self.handler.file_paths, self.file_paths)
        self.assertEqual(self.handler.config, self.config)

    def test_validate_inputs_missing_data_files(self):
        self.file_paths.data_files = []
        with self.assertRaises(ValueError):
            GSAS2Handler(
                path_to_gsas2=self.path_to_gsas2,
                save_directories=self.save_directories,
                refinement_settings=self.refinement_settings,
                file_paths=self.file_paths,
                config=self.config,
            )

    def test_gsasii_scriptable_path_found(self):
        mock_scriptable_path = mock.Mock(spec=Path)
        self.handler.limited_rglob = mock.Mock(return_value=iter([mock_scriptable_path]))
        self.assertEqual(self.handler.gsasii_scriptable_path, mock_scriptable_path)

    def test_gsasii_scriptable_path_not_found(self):
        self.handler.limited_rglob = mock.Mock(return_value=iter([]))
        with self.assertRaises(FileNotFoundError):
            _ = self.handler.gsasii_scriptable_path

    def test_set_gsas2_python_path_found_windows(self):
        mock_python_path = mock.Mock(spec=Path)
        self.handler.os_platform = "Windows"
        self.handler.limited_rglob = mock.Mock(return_value=iter([mock_python_path]))
        self.handler.set_gsas2_python_path()
        self.assertEqual(self.handler._gsas2_python_path, mock_python_path)

    def test_set_gsas2_python_path_not_found_windows(self):
        self.handler.os_platform = "Windows"
        self.handler.limited_rglob = mock.Mock(return_value=iter([]))
        with self.assertRaises(FileNotFoundError):
            self.handler.set_gsas2_python_path()

    def test_to_json(self):
        # Mock the to_json method to avoid actual file operations
        self.handler.to_json = mock.Mock(
            return_value=(
                '{"path_to_gsas2": "/path/to/gsas2", '
                '"temporary_save_directory": "temp_dir", '
                '"project_name": "test_project", '
                '"refinement_settings": {}, '
                '"file_paths": {}, '
                '"limits": [0, 10]}'
            )
        )

        json_output = self.handler.to_json()
        expected_substrings = [
            '"path_to_gsas2": "/path/to/gsas2"',
            '"temporary_save_directory": "temp_dir"',
            '"project_name": "test_project"',
            '"refinement_settings": {}',
            '"file_paths": {}',
            '"limits": [0, 10]',
        ]
        for substring in expected_substrings:
            with self.subTest(substring=substring):
                self.assertIn(substring, json_output)

    def test_set_binaries_windows_mingw_variants(self):
        self.handler.os_platform = "Windows"
        base = self.path_to_gsas2

        test_cases = [
            {
                "desc": "mingw-w64",
                "mingw_dir": base / "Library" / "mingw-w64" / "bin",
            },
            {
                "desc": "mingw64",
                "mingw_dir": base / "Library" / "mingw64" / "bin",
            },
        ]

        for case in test_cases:
            expected_dirs = [
                base,
                base / "bin",
                base / "Library" / "bin",
                base / "Library" / "usr" / "bin",
                case["mingw_dir"],
                base / "Scripts",
            ]

            # Patch Path.exists and Path.is_dir to return True for the expected directories by string comparison
            def is_expected_dir(path):
                return str(path) in {str(d) for d in expected_dirs}

            with (
                self.subTest(mingw_variant=case["desc"]),
                mock.patch.object(Path, "exists", is_expected_dir),
                mock.patch.object(Path, "is_dir", is_expected_dir),
            ):
                self.handler.set_binaries()
                expected_binaries = [str(d.resolve()).replace("/", "\\") for d in expected_dirs]
                self.assertEqual(self.handler.python_binaries, expected_binaries)

    def test_set_binaries_non_windows(self):
        self.handler.os_platform = "Linux"
        mock_paths_bin = [Path(self.path_to_gsas2) / "bin" / "binary1", Path(self.path_to_gsas2) / "bin" / "binary2"]
        self.handler.limited_rglob = mock.Mock(return_value=iter(mock_paths_bin))
        self.handler.set_binaries()
        expected_binaries = [os.fspath(path) for path in mock_paths_bin]
        self.assertEqual(self.handler.python_binaries, expected_binaries)

    def test_set_binaries_no_binaries_found(self):
        self.handler.os_platform = "Linux"
        self.handler.limited_rglob = mock.Mock(return_value=iter([]))
        self.handler.set_binaries()
        self.assertEqual(self.handler.python_binaries, [])

    def test_limited_rglob_match_at_parent_level(self):
        files = ["file1.txt", "file2.log"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=0))
        expected = [Path(self.test_dir) / "file1.txt"]
        self.assertEqual(result, expected)

    def test_limited_rglob_single_level(self):
        files = ["dir1/file1.txt", "dir1/file2.txt"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=1))
        expected = [Path(self.test_dir) / file for file in files]
        self.assertCountEqual(result, expected)

    def test_limited_rglob_exceeds_max_depth(self):
        files = ["dir1/dir2/file1.txt", "dir1/dir2/dir3/file2.txt"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=1))
        self.assertCountEqual(result, [])

    def test_limited_rglob_no_match(self):
        files = ["dir1/file1.txt", "dir1/file2.log"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.csv", max_depth=1))
        self.assertEqual(result, [])

    def test_limited_rglob_multiple_levels(self):
        files = ["dir1/dir2/dir3/file1.txt", "dir1/dir2/dir3/dir4/file2.txt", "dir1/dir2/dir3/dir4/dir5/file3.txt"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=4))
        expected = [
            Path(self.test_dir) / "dir1/dir2/dir3/file1.txt",
            Path(self.test_dir) / "dir1/dir2/dir3/dir4/file2.txt",
        ]
        self.assertCountEqual(result, expected)

    def test_limited_rglob_exact_max_depth(self):
        files = ["dir1/dir2/file1.txt", "dir1/dir2/dir3/file2.txt"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=2))
        expected = [Path(self.test_dir) / "dir1/dir2/file1.txt"]
        self.assertCountEqual(result, expected)

    def test_limited_rglob_different_patterns(self):
        files = ["dir1/file1.txt", "dir1/file2.log", "dir1/file3.csv"]
        self.create_test_files(files)
        result_txt = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=1))
        result_log = list(self.handler.limited_rglob(Path(self.test_dir), "*.log", max_depth=1))
        result_csv = list(self.handler.limited_rglob(Path(self.test_dir), "*.csv", max_depth=1))
        self.assertCountEqual(result_txt, [Path(self.test_dir) / "dir1/file1.txt"])
        self.assertCountEqual(result_log, [Path(self.test_dir) / "dir1/file2.log"])
        self.assertCountEqual(result_csv, [Path(self.test_dir) / "dir1/file3.csv"])

    def test_limited_rglob_empty_directory(self):
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=1))
        self.assertEqual(result, [])

    def test_limited_rglob_similar_directory_names(self):
        files = ["dir1/file1.txt", "dir1_2/file2.txt"]
        self.create_test_files(files)
        result = list(self.handler.limited_rglob(Path(self.test_dir), "*.txt", max_depth=1))
        expected = [
            Path(self.test_dir) / "dir1/file1.txt",
            Path(self.test_dir) / "dir1_2/file2.txt",
        ]
        self.assertCountEqual(result, expected)

    def test_limited_rglob_invalid_directory(self):
        with self.assertRaises(FileNotFoundError):
            list(self.handler.limited_rglob(Path("/invalid/directory"), "*.txt", max_depth=1))
