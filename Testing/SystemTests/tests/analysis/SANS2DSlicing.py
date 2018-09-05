#pylint: disable=invalid-name,attribute-defined-outside-init

from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


MASKFILE = FileFinder.getFullPath('MaskSANS2DReductionGUI.txt')
BATCHFILE = FileFinder.getFullPath('sans2d_reduction_gui_batch.csv')


class SANS2DMinimalBatchReductionSliced(stresstesting.MantidStressTest):
    def __init__(self):
        super(SANS2DMinimalBatchReductionSliced, self).__init__()
        config['default.instrument']='SANS2D'

    def runTest(self):
        import SANSBatchMode as batch
        ii.SANS2D()
        ii.MaskFile(MASKFILE)
        ii.SetEventSlices("0.0-451, 5-10")
        batch.BatchReduce(BATCHFILE, '.nxs',saveAlgs={}, combineDet='rear')

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_reller=True
        self.disableChecking.append('Instrument')
        return str(mtd['trans_test_rear_1D_1.5_12.5'][0]), 'SANSReductionGUI.nxs'


class SANS2DMinimalSingleReductionSliced(SANS2DMinimalBatchReductionSliced):
    def runTest(self):
        ii.SANS2D()
        ii.MaskFile(MASKFILE)
        ii.AssignSample('22048')
        ii.AssignCan('22023')
        ii.TransmissionSample('22041','22024')
        ii.TransmissionCan('22024', '22024')
        ii.SetEventSlices("0.0-450, 5-10")
        reduced = ii.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear_1D_1.5_12.5')


class SANS2DMinimalBatchReductionSlicedTest_V2(stresstesting.MantidStressTest):
    def __init__(self):
        super(SANS2DMinimalBatchReductionSlicedTest_V2, self).__init__()

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile(MASKFILE)
        ii2.SetEventSlices("0.0-451, 5-10")
        ii2.BatchReduce(BATCHFILE, '.nxs', saveAlgs={}, combineDet='rear')

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_reller=True
        self.disableChecking.append('Instrument')
        try:
            return str(AnalysisDataService['trans_test_rear'][0]), 'SANSReductionGUI.nxs'
        except KeyError:
            return '', 'SANSReductionGUI.nxs'


class SANS2DMinimalSingleReductionSlicedTest_V2(stresstesting.MantidStressTest):
    def __init__(self):
        super(SANS2DMinimalSingleReductionSlicedTest_V2, self).__init__()

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile(MASKFILE)
        ii2.AssignSample('22048')
        ii2.AssignCan('22023')
        ii2.TransmissionSample('22041', '22024')
        ii2.TransmissionCan('22024', '22024')
        ii2.SetEventSlices("0.0-450, 5-10")
        reduced = ii2.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_reller=True
        self.disableChecking.append('Instrument')
        return str(AnalysisDataService['trans_test_rear'][0]), 'SANSReductionGUI.nxs'


if __name__ == "__main__":
    test = SANS2DMinimalSingleReductionSliced()
    test.execute()
