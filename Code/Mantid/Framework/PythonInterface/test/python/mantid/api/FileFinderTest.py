import unittest
from mantid.api import FileFinder
import os

class FileFinderTest(unittest.TestCase):

    def test_full_path_returns_an_absolute_path_and_the_files_exists(self):
        path = FileFinder.getFullPath("CNCS_7860_event.nxs")
        self.assertTrue(len(path) > 0)
        # We can't be sure what the full path is in general but it should certainly exist!
        self.assertTrue(os.path.exists(path))

    def test_find_runs_returns_absolute_paths_of_given_runs(self):
        runs = FileFinder.findRuns("CNCS7860")
        self.assertTrue(len(runs) == 1)
        # We can't be sure what the full path is in general but it should certainly exist!
        self.assertTrue(os.path.exists(runs[0]))

if __name__ == '__main__': unittest.main()