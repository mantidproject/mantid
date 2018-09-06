#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
import stresstesting
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


class SANS2DWaveloops(stresstesting.MantidStressTest):

    def runTest(self):

        ii.SANS2D()
        ii.MaskFile('MASKSANS2D.091A')
        ii.Gravity(True)
        ii.Set1D()

        ii.AssignSample('992.raw')
        ii.TransmissionSample('988.raw', '987.raw')
        ii.AssignCan('993.raw')
        ii.TransmissionCan('989.raw', '987.raw')

        ii.CompWavRanges([3, 5, 7, 11], False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
    # testing one of the workspaces that is produced, best not to choose the
    # first one in produced by the loop as this is the least error prone
        return '992rear_1D_7.0_11.0','SANS2DWaveloops.nxs'


class SANS2DWaveloopsTest_V2(stresstesting.MantidStressTest):
    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile('MASKSANS2D.091A')
        ii2.Gravity(True)
        ii2.Set1D()

        ii2.AssignSample('992.raw')
        ii2.TransmissionSample('988.raw', '987.raw')
        ii2.AssignCan('993.raw')
        ii2.TransmissionCan('989.raw', '987.raw')
        ii2.CompWavRanges([3, 5, 7, 11], False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        # testing one of the workspaces that is produced, best not to choose the
        # first one in produced by the loop as this is the least error prone
        return '992rear_1D_7.0_11.0', 'SANS2DWaveloops.nxs'
