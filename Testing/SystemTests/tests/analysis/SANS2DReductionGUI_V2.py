#pylint: disable=invalid-name,attribute-defined-outside-init
"""
The tests here are ports from the original SANS2DReductionGUI.py test suite. Not all tests can be ported since they
include details about the ReductionSingleton
"""

from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.kernel import (config)
from mantid.api import (FileFinder)
from mantid.simpleapi import RenameWorkspace
from sans.command_interface.ISISCommandInterface import (BatchReduce, SANS2D, MaskFile, AssignSample, AssignCan,
                                                         TransmissionSample, TransmissionCan, WavRangeReduction,
                                                         UseCompatibilityMode)

MASKFILE = FileFinder.getFullPath('MaskSANS2DReductionGUI.txt')
BATCHFILE = FileFinder.getFullPath('sans2d_reduction_gui_batch.csv')


class SANS2DMinimalBatchReductionTest_V2(stresstesting.MantidStressTest):
    """Minimal script to perform full reduction in batch mode
    """
    def __init__(self):
        super(SANS2DMinimalBatchReductionTest_V2, self).__init__()
        config['default.instrument'] = 'SANS2D'
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile(MASKFILE)
        BatchReduce(BATCHFILE, '.nxs', combineDet='rear')

    def validate(self):
        self.disableChecking.append('Instrument')
        return "trans_test_rear", "SANSReductionGUI.nxs"


class SANS2DMinimalSingleReductionTest_V2(stresstesting.MantidStressTest):
    """Minimal script to perform full reduction in single mode"""

    def __init__(self):
        super(SANS2DMinimalSingleReductionTest_V2, self).__init__()
        config['default.instrument'] = 'SANS2D'
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile(MASKFILE)
        AssignSample('22048')
        AssignCan('22023')
        TransmissionSample('22041', '22024')
        TransmissionCan('22024', '22024')
        reduced = WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')

    def validate(self):
        self.disableChecking.append('Instrument')
        return "trans_test_rear", "SANSReductionGUI.nxs"
