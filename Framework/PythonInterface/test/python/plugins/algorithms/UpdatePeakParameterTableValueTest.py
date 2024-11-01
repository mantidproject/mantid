# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService


class UpdatePeakParameterTableValueTest(unittest.TestCase):
    def test_updateDouble(self):
        """Test for update a double value"""
        # tablews = self.create_TableWorkspace()

        alg_init = run_algorithm("CreateEmptyTableWorkspace", OutputWorkspace="TestTableWorkspace")
        self.assertTrue(alg_init.isExecuted())

        tablews = AnalysisDataService.retrieve("TestTableWorkspace")

        tablews.addColumn("str", "Name")
        tablews.addColumn("double", "Value")
        tablews.addColumn("str", "FitOrTie")

        tablews.addRow(["A", 1.34, "Fit"])
        tablews.addRow(["B", 2.34, "Tie"])
        tablews.addRow(["S", 3.34, "Tie"])

        alg_test = run_algorithm(
            "UpdatePeakParameterTableValue",
            InputWorkspace=alg_init.getPropertyValue("OutputWorkspace"),
            Column="Value",
            ParameterNames=["A"],
            NewFloatValue=1.00,
        )

        self.assertTrue(alg_test.isExecuted())

        newvalue_A = tablews.cell(0, 1)

        self.assertEqual(newvalue_A, 1.00)

        return


if __name__ == "__main__":
    unittest.main()
