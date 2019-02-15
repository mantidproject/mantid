# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,attribute-defined-outside-init

from __future__ import (absolute_import, division, print_function)
import systemtesting
from mantid.api import (AnalysisDataService, FileFinder)
from sans.command_interface.ISISCommandInterface import (SANS2D, MaskFile, BatchReduce, SetEventSlices,
                                                         UseCompatibilityMode, AssignSample, AssignCan,
                                                         TransmissionSample, TransmissionCan, WavRangeReduction)
from mantid.simpleapi import RenameWorkspace

MASKFILE = FileFinder.getFullPath('MaskSANS2DReductionGUI.txt')
BATCHFILE = FileFinder.getFullPath('sans2d_reduction_gui_batch.csv')


class SANS2DMinimalBatchReductionSlicedTest_V2(systemtesting.MantidSystemTest):
    def __init__(self):
        super(SANS2DMinimalBatchReductionSlicedTest_V2, self).__init__()

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile(MASKFILE)
        SetEventSlices("0.0-451, 5-10")
        BatchReduce(BATCHFILE, '.nxs', saveAlgs={}, combineDet='rear')

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_rel_err = True
        self.disableChecking.append('Instrument')
        try:
            return str(AnalysisDataService['trans_test_rear'][0]), 'SANSReductionGUI.nxs'
        except KeyError:
            return '', 'SANSReductionGUI.nxs'


class SANS2DMinimalSingleReductionSlicedTest_V2(systemtesting.MantidSystemTest):
    def __init__(self):
        super(SANS2DMinimalSingleReductionSlicedTest_V2, self).__init__()

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile(MASKFILE)
        AssignSample('22048')
        AssignCan('22023')
        TransmissionSample('22041', '22024')
        TransmissionCan('22024', '22024')
        SetEventSlices("0.0-450, 5-10")
        reduced = WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_rel_err = True
        self.disableChecking.append('Instrument')
        return str(AnalysisDataService['trans_test_rear'][0]), 'SANSReductionGUI.nxs'

# if __name__ == "__main__":
#     test = SANS2DMinimalBatchReductionSlicedTest_V2()
#     test.execute()
