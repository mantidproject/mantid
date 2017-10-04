from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import CompareWorkspaces, LoadNexusProcessed,\
PowderDiffILLReduction, PowderDiffILLCalibration, SaveNexusProcessed, SaveFocusedXYE
from mantid import config, mtd


class ILLPowderDiffReductionTest(stresstesting.MantidStressTest):

    def __init__(self):
        super(ILLPowderDiffReductionTest, self).__init__()
        self.setUp()

    def setUP(self):
        config.setFacility('ILL')
        config.appendDataSearchSubDir('ILL/D20')

    def requiredFiles(self):
        return ['967087.nxs', '967088.nxs', '967076.nxs']

    def tearDown(self):
        mtd.clear()

    def test_something(self):
        pass
