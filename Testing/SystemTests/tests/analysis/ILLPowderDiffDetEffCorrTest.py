from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import PowderDiffILLDetEffCorr, GroupWorkspaces
from mantid import config, mtd


class ILLPowderDiffDetEffCorrTest(stresstesting.MantidStressTest):

    def __init__(self):
        super(ILLPowderDiffDetEffCorrTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def requiredFiles(self):
        return ['967076.nxs']

    def tearDown(self):
        mtd.clear()

    def runTest(self):

        PowderDiffILLDetEffCorr(CalibrationRun='967076.nxs',
                                OutputWorkspace='calib',
                                OutputResponseWorkspace='response')
        GroupWorkspaces(InputWorkspaces=['calib','response'], OutputWorkspace='group')

    def validate(self):
        self.tolerance = 0.0001
        return ['group', 'ILL_D20_calib_def.nxs']
