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
from mantid.api import ITableWorkspace


class WorkspaceValidatorsTest(unittest.TestCase):

    def test_TableWorkspaceNotEmptyValidator_construction(self):
        """
        Test that the TableWorkspaceNotEmptyValidator can be constructed
        and detect an empty TableWorkspace
        """

        tableWS = CreateEmptyTableWorkspace()

        # test the validator can be constructed
        testhelpers.assertRaisesNothing(self, TableWorkspaceNotEmptyValidator)

        # test the validator rejects an empty TableWorkspace
        self.assertRaises(Exception, TableWorkspaceNotEmptyValidator, tableWS)

        # thest the validator accepts a filled TableWorkspace
        tableWS.addColumn("int", "values")
        tableWS.addRow([1])
        tableWS.addRow([2])
        tableWS.addRow([3])

        testhelpers.assertRaisesNothing(self, TableWorkspaceNotEmptyValidator)


if __name__ == "__main__":
    unittest.main()
