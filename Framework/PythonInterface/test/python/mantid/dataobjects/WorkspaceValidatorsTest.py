# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
   Test construction of the WorkspaceValidators
"""
import unittest
import testhelpers

from mantid.dataobjects import TableWorkspaceNotEmptyValidator

from mantid.simpleapi import CreateEmptyTableWorkspace
from mantid.api import ITableWorkspace, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction


class WorkspaceValidatorsTest(unittest.TestCase):

    def test_TableWorkspaceNotEmptyValidator_construction(self):
        """
        Test that the TableWorkspaceNotEmptyValidator can be constructed
        """

        # test the validator can be constructed without error
        testhelpers.assertRaisesNothing(self, TableWorkspaceNotEmptyValidator)

    def test_TableWorkspaceNotEmptyValidator_usage(self):

        class TestAlgorithm(PythonAlgorithm):
            """Mock an extended algorithm that uses TableWorkspaceNotEmptyValidator"""

            def PyInit(self):
                self.declareProperty(
                    WorkspaceProperty(
                        name="InputWorkspace", defaultValue="", direction=Direction.Input, validator=TableWorkspaceNotEmptyValidator()
                    )
                )

            def PyExec(self):
                pass

        # setup the algorithm
        alg = TestAlgorithm()
        alg.PyInit()

        # setup the table workspace
        tableWS = CreateEmptyTableWorkspace()

        # test the validator rejects an 0-column 0-row TableWorkspace
        self.assertRaises(ValueError, alg.setProperty, "InputWorkspace", tableWS)

        # test the validator rejects a n-column, 0-row TableWorkspace
        tableWS.addColumn("int", "values0")
        self.assertRaises(ValueError, alg.setProperty, "InputWorkspace", tableWS)

        tableWS.addColumn("int", "values1")
        tableWS.addColumn("int", "values2")
        self.assertRaises(ValueError, alg.setProperty, "InputWorkspace", tableWS)

        # test the validator accepts a filled TableWorkspace
        tableWS.addRow([1, 2, 3])
        tableWS.addRow([4, 5, 6])
        testhelpers.assertRaisesNothing(self, alg.setProperty, "InputWorkspace", tableWS)


if __name__ == "__main__":
    unittest.main()
