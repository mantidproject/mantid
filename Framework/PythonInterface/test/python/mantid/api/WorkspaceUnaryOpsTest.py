# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import mtd
from mantid.simpleapi import CreateMDHistoWorkspace
import unittest


class WorkspaceUnaryOpsTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def test_unary_ops_with_workspaces_not_in_ADS(self):
        mdws = CreateMDHistoWorkspace(
            SignalInput=[0], ErrorInput=[0], Dimensionality=1, Extents=[0, 1], NumberOfBins=1, Names=["a"], Units=["TOF"], StoreInADS=False
        )
        mdws_ads = CreateMDHistoWorkspace(
            SignalInput=[0], ErrorInput=[0], Dimensionality=1, Extents=[0, 1], NumberOfBins=1, Names=["a"], Units=["TOF"], StoreInADS=True
        )
        result1 = ~mdws  # noqa: F841
        self.assertTrue(mtd.doesExist("result1"))
        result2 = ~mdws_ads  # noqa: F841
        self.assertTrue(mtd.doesExist("result2"))


if __name__ == "__main__":
    unittest.main()
