# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# local
from mantid import config
from mantid.utils import path

# standard
from pathlib import Path
import unittest
import tempfile


class PathTest(unittest.TestCase):
    def test_run_exists(self):
        # create fake events file in temporary directory
        data_dir = tempfile.gettempdir()
        old = config["datasearch.directories"]
        config["datasearch.directories"] = f"{data_dir};{old}"
        expected = Path(data_dir) / "SNAP_45874.nxs.h5"
        expected.touch()  # create empty file
        # Normalize path separators for comparison (ConfigService returns paths with forward slashes)
        actual = Path(path.run_file(45874, instrument="SNAP", oncat=False))
        self.assertEqual(actual, expected)
        config["datasearch.directories"] = old  # restore the original list of data search directories


if __name__ == "__main__":
    unittest.main()
