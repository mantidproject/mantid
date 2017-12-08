from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import CreateSampleWorkspace
import unittest


class WorkspaceBinaryOpsTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def test_binary_ops_with_workspaces_not_in_ADS(self):
        ws = CreateSampleWorkspace(StoreInADS=False)
        initialY = ws.readY(0)[0]
        ws_ads = CreateSampleWorkspace(StoreInADS=True)
        result1 = ws + ws
        self.assertTrue(mtd.doesExist('result1'))
        result2 = ws + ws_ads
        self.assertTrue(mtd.doesExist('result2'))
        result3 = ws_ads + ws
        self.assertTrue(mtd.doesExist('result3'))
        result4 = ws_ads + ws_ads
        self.assertTrue(mtd.doesExist('result4'))
        result5 = ws + 1
        self.assertTrue(mtd.doesExist('result5'))
        result6 = 1 + ws
        self.assertTrue(mtd.doesExist('result6'))
        result7 = ws_ads + 1
        self.assertTrue(mtd.doesExist('result7'))
        result8 = 1 + ws_ads
        self.assertTrue(mtd.doesExist('result8'))
        ws += 1
        self.assertFalse(mtd.doesExist('ws'))
        ws_ads += 1
        self.assertTrue(mtd.doesExist('ws_ads'))

if __name__ == '__main__':
    unittest.main()
