from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import PowderDiffILLDetEffCorr, GroupWorkspaces
from mantid import config, mtd
import numpy as np


class ILL_D2B_DetEffCorrTest(stresstesting.MantidStressTest):

    def __init__(self):
        super(ILL_D2B_DetEffCorrTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D2B'
        config.appendDataSearchSubDir('ILL/D2B/')

    def requiredFiles(self):
        return ['532008.nxs', '532009.nxs']

    def tearDown(self):
        mtd.clear()

    def testAutoMasking(self):
        PowderDiffILLDetEffCorr(CalibrationRun='532008,532009',
                                DerivationMethod='GlobalSummedReference2D',
                                ExcludedRange=[-5,10],
                                OutputWorkspace='masked',
                                MaskCriterion=[0.3,3])
        data = mtd['masked'].extractY().flatten()
        data = data[np.nonzero(data)]
        coeff_max = data.max()
        self.assertTrue(coeff_max <= 3.)
        coeff_min = data.min()
        self.assertTrue(coeff_min >= 0.3)

    def runTest(self):

        self.testAutoMasking()

        PowderDiffILLDetEffCorr(CalibrationRun='532008,532009',
                                DerivationMethod='GlobalSummedReference2D',
                                ExcludedRange=[-5,10],
                                OutputWorkspace='calib',
                                OutputResponseWorkspace='response')
        GroupWorkspaces(InputWorkspaces=['calib','response'], OutputWorkspace='group')

    def validate(self):
        self.tolerance = 0.01
        return ['group', 'D2B_DetEffCorr_Ref.nxs']
