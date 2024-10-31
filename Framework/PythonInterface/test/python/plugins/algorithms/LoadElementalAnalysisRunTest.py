# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.api import AnalysisDataService
from mantid.simpleapi import AlgorithmManager


class LoadElementalAnalysisRunTest(unittest.TestCase):
    def test_incorrect_run_number(self):
        alg = AlgorithmManager.create("LoadElementalAnalysisData")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("Run", 1)
        alg.setProperty("GroupWorkspace", "1")
        errors = alg.validateInputs()
        self.assertTrue("Run" in errors)
        self.assertEqual(len(errors), 1)
        self.assertFalse(AnalysisDataService.doesExist("1"))


if __name__ == "__main__":
    unittest.main()
