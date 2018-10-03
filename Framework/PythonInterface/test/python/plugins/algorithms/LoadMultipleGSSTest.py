# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

class LoadMultipleGSSTest(unittest.TestCase):

    def test_LoadMultipleGSSTest(self):
        # Set up
        alg_test = run_algorithm("LoadMultipleGSS",
                                 FilePrefix = "PG3",
                                 RunNumbers = [11485,11486],
                                 Directory  = "")
        # Execute
        self.assertTrue(alg_test.isExecuted())

        # just make sure there are output workspaces then delete them
        for name in ["PG3_11485", "PG3_11486"]:
            wksp = AnalysisDataService.retrieve(name)
            self.assertTrue(wksp is not None)
            AnalysisDataService.remove(name)

if __name__ == '__main__':
    unittest.main()
