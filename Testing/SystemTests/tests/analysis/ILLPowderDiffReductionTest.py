from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import PowderDiffILLReduction
from mantid import config, mtd


class ILLPowderDiffReductionTest(stresstesting.MantidStressTest):

    def __init__(self):
        super(ILLPowderDiffReductionTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def requiredFiles(self):
        return ['967087.nxs', '967088.nxs']

    def tearDown(self):
        mtd.clear()

    def runTest(self):
        PowderDiffILLReduction(Run='967087,967088',OutputWorkspace='reduced')

    def validate(self):
        self.tolerance = 0.0001
        # something goes wrong with DetInfo when saving loading nexus processed
        self.disableChecking.append("Instrument")
        return ['reduced', 'ILL_D20_red_def.nxs']
