# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryDirectory
from time import sleep
from typing import List
from unittest import TestCase
from unittest.mock import MagicMock, patch

from mantid.kernel.environment import is_linux
from mantidqt.dialogs.errorreports.run_pystack import (
    _get_core_dumps_dir,
    _get_pystack_output_for_most_recent_core_dump,
    _get_pystack_output_if_workbench_process,
    _is_lz4_file,
    _is_workbench_process,
    _run_pystack,
)

if is_linux():
    import lz4.frame
import os

PYSTACK_OUTPUT = (
    "Using executable found in the core file: /a/conda/env/bin/python\n\n"
    "Core file information:\n"
    "state: R zombie: True niceness: 0\n"
    "pid: 1234 ppid: 1200 sid: 1100\n"
    "Traceback for thread 1234 [] (most recent call last):\n"
)


class TestRunPystack(TestCase):
    MODULE_PATH = "mantidqt.dialogs.errorreports.run_pystack"

    def setUp(self) -> None:
        if not is_linux():
            self.skipTest("pystack is only run on linux")

    @patch(f"{MODULE_PATH}.ConfigService")
    def test_get_core_dumps_dir_raises_if_not_set(self, mock_config_service: MagicMock):
        mock_config_service.getString.return_value = ""
        self.assertRaisesRegex(ValueError, "errorreports.core_dumps not set", _get_core_dumps_dir)

    @patch(f"{MODULE_PATH}.ConfigService")
    def test_get_core_dumps_dir_raises_if_does_not_exist(self, mock_config_service: MagicMock):
        mock_config_service.getString.return_value = "/a/fake/path"
        self.assertRaisesRegex(ValueError, "does not exist", _get_core_dumps_dir)

    @patch(f"{MODULE_PATH}.ConfigService")
    def test_get_core_dumps_dir_raises_if_file_is_set(self, mock_config_service: MagicMock):
        with NamedTemporaryFile() as tmp_file:
            mock_config_service.getString.return_value = tmp_file.name
            self.assertRaisesRegex(ValueError, "is not a directory", _get_core_dumps_dir)

    @patch(f"{MODULE_PATH}.ConfigService")
    def test_get_core_dumps_dir_returns_a_dir_set_in_the_config(self, mock_config_service: MagicMock):
        with TemporaryDirectory() as tmp_dir:
            mock_config_service.getString = MagicMock()
            mock_config_service.getString.return_value = tmp_dir
            path = _get_core_dumps_dir()
            mock_config_service.getString.assert_called_once_with("errorreports.core_dumps")
            self.assertEqual(path.as_posix(), tmp_dir)

    @patch(f"{MODULE_PATH}._get_pystack_output_if_workbench_process")
    def test_get_pystack_output_for_most_recent_core_dump_uses_the_latest_file(self, mock_get_output: MagicMock):
        mock_get_output.return_value = PYSTACK_OUTPUT
        file_names = ["first", "second", "third"]
        with SetupSomeFilesInATempDir(file_names) as tmp_dir:
            output = _get_pystack_output_for_most_recent_core_dump(Path(tmp_dir), "1234")
            self.assertEqual(output, PYSTACK_OUTPUT)
            mock_get_output.assert_called_once_with(Path(tmp_dir) / file_names[-1], "1234")

    @patch(f"{MODULE_PATH}._get_pystack_output_if_workbench_process")
    def test_get_pystack_output_for_most_recent_core_dump_skips_files_from_other_processes(self, mock_get_output: MagicMock):
        mock_get_output.side_effect = ["", PYSTACK_OUTPUT]
        file_names = ["first", "second", "third"]
        with SetupSomeFilesInATempDir(file_names) as tmp_dir:
            output = _get_pystack_output_for_most_recent_core_dump(Path(tmp_dir), "1234")
            self.assertEqual(output, PYSTACK_OUTPUT)
            self.assertEqual(mock_get_output.call_count, 2)
            mock_get_output.assert_called_with(Path(tmp_dir) / file_names[-2], "1234")

    @patch(f"{MODULE_PATH}._get_pystack_output_if_workbench_process")
    @patch(f"{MODULE_PATH}.CORE_DUMP_RECENCY_LIMIT", 0.5)
    def test_get_pystack_output_for_most_recent_core_dump_returns_empty_if_there_are_no_new_files(self, mock_get_output: MagicMock):
        with TemporaryDirectory() as tmp_dir:
            open(f"{tmp_dir}/test", "a").close()
            sleep(0.6)
            self.assertEqual(_get_pystack_output_for_most_recent_core_dump(Path(tmp_dir), "1234"), "")
            mock_get_output.assert_not_called()

    def test_get_pystack_output_for_most_recent_core_dump_returns_empty_if_the_dir_is_empty(self):
        with TemporaryDirectory() as tmp_dir:
            self.assertEqual(_get_pystack_output_for_most_recent_core_dump(Path(tmp_dir), "1234"), "")

    @patch(f"{MODULE_PATH}._run_pystack")
    @patch(f"{MODULE_PATH}._is_lz4_file")
    def test_get_pystack_output_if_workbench_process_returns_output_for_workbench_core(
        self, mock_is_lz4_file: MagicMock, mock_run_pystack: MagicMock
    ):
        mock_is_lz4_file.return_value = False
        mock_run_pystack.return_value = PYSTACK_OUTPUT
        core_file = Path("/a/core/file")
        self.assertEqual(_get_pystack_output_if_workbench_process(core_file, "1234"), PYSTACK_OUTPUT)
        mock_run_pystack.assert_called_once_with(core_file)

    @patch(f"{MODULE_PATH}._run_pystack")
    @patch(f"{MODULE_PATH}._is_lz4_file")
    def test_get_pystack_output_if_workbench_process_returns_empty_for_other_process(
        self, mock_is_lz4_file: MagicMock, mock_run_pystack: MagicMock
    ):
        mock_is_lz4_file.return_value = False
        mock_run_pystack.return_value = PYSTACK_OUTPUT
        self.assertEqual(_get_pystack_output_if_workbench_process(Path("/a/core/file"), "999"), "")

    @patch(f"{MODULE_PATH}._run_pystack")
    @patch(f"{MODULE_PATH}._decompress_lz4_file")
    @patch(f"{MODULE_PATH}._is_lz4_file")
    def test_decompressed_lz4_file_is_analysed_and_then_removed(
        self, mock_is_lz4_file: MagicMock, mock_decompress_lz4_file: MagicMock, mock_run_pystack: MagicMock
    ):
        mock_is_lz4_file.return_value = True
        mock_run_pystack.return_value = PYSTACK_OUTPUT
        with NamedTemporaryFile(delete=False) as tmp_file:
            decompressed_file = Path(tmp_file.name)
        mock_decompress_lz4_file.return_value = decompressed_file
        core_file = Path("/a/core/file.lz4")

        self.assertEqual(_get_pystack_output_if_workbench_process(core_file, "1234"), PYSTACK_OUTPUT)

        mock_decompress_lz4_file.assert_called_once_with(core_file)
        mock_run_pystack.assert_called_once_with(decompressed_file)
        self.assertFalse(decompressed_file.exists())

    @patch(f"{MODULE_PATH}.subprocess")
    def test_run_pystack_returns_stdout_even_if_there_are_errors_on_stderr(self, mock_subprocess: MagicMock):
        mock_subprocess.run.return_value = MagicMock(
            stdout=PYSTACK_OUTPUT, stderr="Could not gather enough information to extract the Python frame information"
        )
        core_file = Path("/a/core/file")
        self.assertEqual(_run_pystack(core_file), PYSTACK_OUTPUT)
        mock_subprocess.run.assert_called_once_with(
            ["pystack", "core", core_file.as_posix(), "--native-all"], capture_output=True, text=True
        )

    def test_is_workbench_process_matches_pid(self):
        self.assertTrue(_is_workbench_process(PYSTACK_OUTPUT, "1234"))

    def test_is_workbench_process_matches_parent_pid(self):
        self.assertTrue(_is_workbench_process(PYSTACK_OUTPUT, "1200"))

    def test_is_workbench_process_is_false_for_a_different_pid(self):
        self.assertFalse(_is_workbench_process(PYSTACK_OUTPUT, "999"))

    def test_is_workbench_process_is_false_without_core_file_information(self):
        self.assertFalse(_is_workbench_process("", "1234"))

    def test_is_lz4_file_is_true_for_lz4_file(self):
        random_data = os.urandom(1024)
        with NamedTemporaryFile() as tmp_file:
            with lz4.frame.open(tmp_file.name, "wb") as fp:
                fp.write(random_data)
            self.assertTrue(_is_lz4_file(Path(tmp_file.name)))

    def test_is_lz4_is_false_for_tmp_file(self):
        with NamedTemporaryFile() as tmp_file:
            self.assertFalse(_is_lz4_file(Path(tmp_file.name)))


class SetupSomeFilesInATempDir:
    def __init__(self, file_names: List[str]):
        self.tmp_dir = TemporaryDirectory()
        for name in file_names:
            open(f"{self.tmp_dir.name}/{name}", "a").close()
            sleep(0.1)

    def __enter__(self):
        return self.tmp_dir.name

    def __exit__(self, type, value, traceback):
        self.tmp_dir.cleanup()
