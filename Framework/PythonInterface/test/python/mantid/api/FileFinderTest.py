# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from mantid.api import FileFinder


class FileFinderTest(unittest.TestCase):
    def test_full_path_returns_an_absolute_path_and_the_files_exists(self):
        path = FileFinder.getFullPath("CNCS_7860_event.nxs")
        self.assertGreater(len(path), 0)
        # We can't be sure what the full path is in general but it should certainly exist!
        self.assertTrue(os.path.exists(path))

    def test_find_runs_returns_absolute_paths_of_given_runs(self):
        runs = FileFinder.findRuns("CNCS7860")
        self.assertEqual(len(runs), 1)
        # We can't be sure what the full path is in general but it should certainly exist!
        self.assertTrue(os.path.exists(runs[0]))

    def test_that_find_runs_accepts_a_list_of_string_and_a_bool(self):
        try:
            runs = FileFinder.findRuns("CNCS7860", useExtsOnly=True)
            FileFinder.findRuns("CNCS7860", [".nxs", ".txt"], useExtsOnly=True)
        except Exception as e:
            if type(e).__name__ == "ArgumentError":
                self.fail(
                    "Expected findRuns to accept a list of strings and a bool as input." " {} error was raised with message {}".format(
                        type(e).__name__, str(e)
                    )
                )
        else:
            # Confirm that it works as above
            self.assertEqual(len(runs), 1)
            self.assertTrue(os.path.exists(runs[0]))


if __name__ == "__main__":
    unittest.main()
