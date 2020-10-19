# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import LoadElementalAnalysisData, GroupWorkspaces

class LoadElementalAnalysisRunTest(unittest.TestCase):

    def set_up_alg(self):
        run = 1
        alg= LoadElementalAnalysisData(Run=run,
                                  GroupWorkspace=run)
        alg.initialize()

    def test_incorrect_run_number(self):
        alg = self.set_up_alg()
        errors = alg.validateInputs()
        self.assertTrue("Cannot find files for run" in errors)
        self.assertEquals(len(errors), 1)

if __name__ == '__main__':
    unittest.main()