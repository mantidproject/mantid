from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import CreateMDHistoWorkspace
import unittest


class WorkspaceUnaryOpsTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def test_unary_ops_with_workspaces_not_in_ADS(self):
        mdws = CreateMDHistoWorkspace(SignalInput=[0], ErrorInput=[0], Dimensionality=1, Extents=[0, 1], NumberOfBins=1, Names=['a'], Units=['TOF'], StoreInADS=False)
        mdws_ads = CreateMDHistoWorkspace(SignalInput=[0], ErrorInput=[0], Dimensionality=1, Extents=[0, 1], NumberOfBins=1, Names=['a'], Units=['TOF'], StoreInADS=True)
        result1 = ~mdws
        self.assertTrue(mtd.doesExist('result1'))
        result2 = ~mdws_ads
        self.assertTrue(mtd.doesExist('result2'))


if __name__ == '__main__':
    unittest.main()
