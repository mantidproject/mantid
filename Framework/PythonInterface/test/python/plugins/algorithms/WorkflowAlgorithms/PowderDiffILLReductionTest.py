from __future__ import (absolute_import, division, print_function)

import os
import unittest
from mantid.simpleapi import PowderDiffILLReduction
from mantid.api import MatrixWorkspace,WorkspaceGroup
from mantid import config, mtd


class PowderDiffILLReductionTest(unittest.TestCase):

    _runs = ['967087.nxs', '967088.nxs']

    def setUP(self):
        pass
    def tearDown(self):
        pass
    def test_something(self):
        pass

if __name__ == '__main__':
    unittest.main()
