# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryDirectory
from time import sleep
from unittest import TestCase
from unittest.mock import MagicMock, patch

from mantid.kernel.environment import is_linux
from mantidqt.dialogs.errorreports.run_pystack import _get_core_dumps_dir, _get_most_recent_core_dump_file


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

    @patch(f"{MODULE_PATH}._check_core_file_is_the_workbench_process")
    def test_get_most_recent_core_dump_file_gets_the_latest_file(self, mock_check_workbench_process: MagicMock):
        mock_check_workbench_process.return_value = True
        file_names = ["first", "second", "third"]
        with TemporaryDirectory() as tmp_dir:
            for name in file_names:
                open(f"{tmp_dir}/{name}", "a").close()
                sleep(0.1)
            latest_file = _get_most_recent_core_dump_file(Path(tmp_dir))
            self.assertEqual(latest_file.name, file_names[-1])

    @patch(f"{MODULE_PATH}.CORE_DUMP_RECENCY_LIMIT", 0.5)
    def test_get_most_recent_core_dump_file_returns_none_if_there_are_no_new_files(self):
        with TemporaryDirectory() as tmp_dir:
            open(f"{tmp_dir}/test", "a").close()
            sleep(0.6)
            self.assertIsNone(_get_most_recent_core_dump_file(Path(tmp_dir)))

    def test_get_most_recent_core_dump_file_returns_none_if_the_dir_is_empty(self):
        with TemporaryDirectory() as tmp_dir:
            self.assertIsNone(_get_most_recent_core_dump_file(Path(tmp_dir)))
