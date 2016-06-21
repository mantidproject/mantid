#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from ISISCommandInterface import *

class SANS2DWaveloops(stresstesting.MantidStressTest):

    def runTest(self):

        SANS2D()
        MaskFile('MASKSANS2D.091A')
        Gravity(True)
        Set1D()

        AssignSample('992.raw')
        TransmissionSample('988.raw', '987.raw')
        AssignCan('993.raw')
        TransmissionCan('989.raw', '987.raw')

        CompWavRanges([3, 5, 7, 11], False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
    # testing one of the workspaces that is produced, best not to choose the
    # first one in produced by the loop as this is the least error prone
        return '992rear_1D_7.0_11.0','SANS2DWaveloops.nxs'
