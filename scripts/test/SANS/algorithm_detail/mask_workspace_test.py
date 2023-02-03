# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans.algorithm_detail.mask_workspace import mask_with_mask_files


class MaskWorkspaceTest(unittest.TestCase):
    @mock.patch("sans.algorithm_detail.mask_workspace.find_full_file_path")
    @mock.patch("sans.algorithm_detail.mask_workspace.create_unmanaged_algorithm")
    def test_raises_if_file_not_found(self, _1, finder):
        mask_info, inst_info = mock.Mock(), mock.Mock()
        mask_info.mask_files = ["filename", "filename2"]
        finder.return_value = None
        with self.assertRaisesRegex(FileNotFoundError, "(?=filename)(?=filename2)"):
            mask_with_mask_files(mask_info, inst_info, workspace=None)

    @mock.patch("sans.algorithm_detail.mask_workspace.find_full_file_path")
    @mock.patch("sans.algorithm_detail.mask_workspace.create_unmanaged_algorithm")
    def test_does_not_raise_if_found(self, _1, finder):
        finder.return_value = "some_path"
        mask_info, inst_info = mock.Mock(), mock.Mock()
        mask_info.mask_files = [mock.Mock(), mock.Mock()]
        self.assertIsNotNone(mask_with_mask_files(mask_info, inst_info, workspace=None))


if __name__ == "__main__":
    unittest.main()
