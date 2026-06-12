# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#

import unittest
import tempfile
from Engineering.texture.TextureUtils.io import find_all_files, mk
import os

texture_utils_path = "Engineering.texture.TextureUtils.io"


class TextureUtilsFileHelperTests(unittest.TestCase):
    def test_find_all_files_returns_only_files(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = os.path.join(tmpdir, "file1.txt")
            dir_path = os.path.join(tmpdir, "subdir")
            os.mkdir(dir_path)
            with open(file_path, "w") as f:
                f.write("test")
            result = find_all_files(tmpdir)
            self.assertIn(file_path, result)
            self.assertNotIn(dir_path, result)

    def test_mk_creates_missing_directory(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            test_path = os.path.join(tmpdir, "newdir")
            self.assertFalse(os.path.exists(test_path))
            mk(test_path)
            self.assertTrue(os.path.exists(test_path))


if __name__ == "__main__":
    unittest.main()
