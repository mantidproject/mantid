import stresstesting

from mantid.simpleapi import PowderDiffILLDetScanReduction
from mantid import config


class PowderDiffILLDetScanReductionTest(stresstesting.MantidStressTest):

    def __init__(self):
        super(PowderDiffILLDetScanReductionTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D2B'
        config.appendDataSearchSubDir('ILL/D2B/')

    def requiredFiles(self):
        return ["508093.nxs"]

    def d2b_2d_test(self):
        _2d = PowderDiffILLDetScanReduction(
            Run='508093-508095',
            Output2D = True,
            Output2DStraight = False,
            Output1D = False,
            OutputWorkspace='outWS')[0]
        return _2d

    def d2b_2dstraight_test(self):
        _2d_straight = PowderDiffILLDetScanReduction(
            Run = '508093-508095',
            UsePrecalibratedData = False,
            NormaliseTo = 'None',
            Output2D = False,
            Output2DStraight = True,
            Output1D = False,
            OutputWorkspace='outWS')[0]
        return _2d_straight

    def d2b_1d_test(self):
        _1d = PowderDiffILLDetScanReduction(
            Run='508093-508095',
            Output2D = False,
            Output2DStraight = False,
            Output1D = True,
            OutputWorkspace='outWS')[0]
        return _1d

    def runTest(self):
        _2d = self.d2b_2d_test()
        _2d_straight = self.d2b_2dstraight_test()
        _1d = self.d2b_1d_test()
        grouped = GroupWorkspaces([_2d, _2d_straight, _1d])

