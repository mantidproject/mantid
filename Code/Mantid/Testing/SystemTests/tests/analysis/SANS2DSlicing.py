#pylint: disable=invalid-name,attribute-defined-outside-init
import sys

if __name__ == "__main__":
  # it is just to allow running this test in Mantid, allowing the following import
    sys.path.append('/apps/mantid/systemtests/StressTestFramework/')

import stresstesting

from mantid.simpleapi import *
import ISISCommandInterface as i

MASKFILE = FileFinder.getFullPath('MaskSANS2DReductionGUI.txt')
BATCHFILE = FileFinder.getFullPath('sans2d_reduction_gui_batch.csv')

class SANS2DMinimalBatchReductionSliced(stresstesting.MantidStressTest):
    def __init__(self):
        super(SANS2DMinimalBatchReductionSliced, self).__init__()
        config['default.instrument']='SANS2D'
    def runTest(self):
        import SANSBatchMode as batch
        i.SANS2D()
        i.MaskFile(MASKFILE)
        i.SetEventSlices("0.0-451, 5-10")
        fit_settings = batch.BatchReduce(BATCHFILE, '.nxs',saveAlgs={}, combineDet='rear')

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_reller=True
        return str(mtd['trans_test_rear'][0]), 'SANSReductionGUI.nxs'

class SANS2DMinimalSingleReductionSliced(SANS2DMinimalBatchReductionSliced):
    def runTest(self):
        i.SANS2D()
        i.MaskFile(MASKFILE)
        i.AssignSample('22048')
        i.AssignCan('22023')
        i.TransmissionSample('22041','22024')
        i.TransmissionCan('22024', '22024')
        i.SetEventSlices("0.0-450, 5-10")
        reduced = i.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')


if __name__ == "__main__":
    test = SANS2DMinimalSingleReductionSliced()
    test.execute()
