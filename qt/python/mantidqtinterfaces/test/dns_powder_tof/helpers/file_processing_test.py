# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS file helpers.
"""

import unittest
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.helpers.file_processing import (
    create_dir,
    create_dir_from_filename,
    filter_filenames,
    get_path_and_prefix,
    load_txt,
    open_editor,
    return_filelist,
    return_standard_zip,
    save_txt,
    unzip_latest_standard,
)
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_3_filenames


def mock_mtime(filename):
    times = {"standard.zip": 5, "standard123.zip": 4}
    return times[filename]


class DNSfile_processingTest(unittest.TestCase):
    def setUp(self):
        self.filenames = get_3_filenames()

    def test_filter_filenames(self):
        filtered = filter_filenames(self.filenames, 774714, 788058)
        self.assertEqual(filtered, self.filenames)
        filtered = filter_filenames(self.filenames, 774714, 788057)
        self.assertEqual(filtered, self.filenames[0:2])
        filtered = filter_filenames(self.filenames, 787464, 788057)
        self.assertEqual(filtered, [])

    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.listdir")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.path.isdir")
    def test_return_filelist(self, mock_is_dir, mock_listdir):
        mock_listdir.return_value = self.filenames + ["nano123.d_dat", "a.ddat", "123.txt"]
        mock_is_dir.return_value = True
        filelist = return_filelist(mock_listdir.return_value)
        self.assertEqual(filelist, sorted(self.filenames + ["nano123.d_dat"]))
        mock_is_dir.return_value = False
        filelist = return_filelist(mock_listdir.return_value)
        self.assertEqual(filelist, [])

    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.path.getmtime", new=mock_mtime)
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.glob.glob")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.path.isdir")
    def test_return_standard_zip(self, mock_is_dir, mock_glob):
        mock_is_dir.return_value = False
        mock_glob.return_value = ["standard.zip", "standard123.zip"]
        test_v = return_standard_zip("a")
        self.assertEqual(test_v, "")
        mock_is_dir.return_value = True
        test_v = return_standard_zip("a")
        self.assertEqual(test_v, "standard.zip")

    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.zipfile.ZipFile")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.return_standard_zip")
    def test_unzip_latest_standard(self, mock_return_standard, mock_zip):
        mock_extract = mock_zip.return_value.__enter__.return_value.extractall
        mock_return_standard.return_value = "standard.zip"
        self.assertTrue(unzip_latest_standard("datab", "stan"))
        mock_zip.assert_called_once_with("standard.zip", "r")
        mock_extract.assert_called_once_with("stan")
        mock_extract.reset_mock()
        mock_return_standard.reset_mock()
        mock_return_standard.return_value = ""
        self.assertFalse(unzip_latest_standard("datab", "stan"))
        mock_extract.assert_not_called()
        self.assertEqual(mock_return_standard.call_count, 2)

    @staticmethod
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.create_dir")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.path.dirname")
    def test_create_dir_from_filename(mock_dir_name, mock_cdir):
        create_dir_from_filename("123")
        mock_dir_name.assert_called_once_with("123")
        mock_cdir.assert_called_once_with(mock_dir_name.return_value)

    @staticmethod
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.makedirs")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.path.exists")
    def test_create_dir(mock_path, mock_make_dir):
        mock_path.return_value = True
        create_dir("a")
        mock_make_dir.assert_not_called()
        mock_path.return_value = False
        create_dir("a")
        mock_make_dir.assert_called_once_with("a")

    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.open")
    def test_save_txt(self, mock_open):
        mock_write = mock_open.return_value.__enter__.return_value.write
        test_v = save_txt(txt="abc", filename="123.dat", current_dir=None)
        self.assertEqual(test_v, ["123.dat", "123.dat"])
        mock_open.assert_called_once_with("123.dat", "w", encoding="utf8")
        mock_write.assert_called_once_with("abc")
        test_v = save_txt(txt="abc", filename="123.dat", current_dir="d")
        self.assertEqual(test_v, ["123.dat", "d/123.dat"])

    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.open")
    def test_load_txt(self, mock_open):
        mock_read = mock_open.return_value.__enter__.return_value.read
        mock_read.return_value = "hzu"
        test_v = load_txt(filename="123.dat", current_dir=None)
        mock_open.assert_called_once_with("123.dat", "r", encoding="utf8")
        mock_read.assert_called_once()
        mock_open.reset_mock()
        mock_read.rset_mock()
        test_v = load_txt(filename="123.dat", current_dir="d")
        mock_open.assert_called_once_with("d/123.dat", "r", encoding="utf8")
        self.assertEqual(test_v, "hzu")

    @staticmethod
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.subprocess.call")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.sys")
    @patch("mantidqtinterfaces.dns_powder_tof.helpers.file_processing.os.path.exists")
    def test_open_editor(mock_path_exist, mock_sys, mock_subprocess):
        mock_path_exist.return_value = False
        mock_sys.platform = "win32"
        open_editor("123.d_dat", current_dir=None)
        mock_subprocess.assert_not_called()
        mock_path_exist.assert_called_once_with("123.d_dat")
        mock_path_exist.return_value = True
        mock_path_exist.reset_mock()
        open_editor("123.d_dat", current_dir="d")
        mock_path_exist.assert_called_once_with("d/123.d_dat")
        mock_subprocess.assert_called_once_with(["cmd.exe", "/c", "d/123.d_dat"])
        mock_subprocess.reset_mock()
        mock_sys.platform = "linux2"
        open_editor("123.d_dat", current_dir="d")
        mock_subprocess.assert_called_with(["xdg-open", "d/123.d_dat"])
        mock_sys.platform = "darwin"
        open_editor("123.d_dat", current_dir="d")
        mock_subprocess.assert_called_with(["open", "d/123.d_dat"])
        mock_subprocess.reset_mock()
        mock_sys.platform = "nonsense"
        open_editor("123.d_dat", current_dir="d")
        mock_subprocess.assert_not_called()

    def test_get_path_and_prefix(self):
        test_v = get_path_and_prefix("C:/abc/123.d_dat")
        self.assertEqual(test_v, ("C:/abc", "123.d_dat"))


if __name__ == "__main__":
    unittest.main()
