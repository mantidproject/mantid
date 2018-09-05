#pylint: disable=invalid-name

from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
import stresstesting
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


class SANSMergedDetectorsTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)

    def runTest(self):
        # Select instrument and user file
        ii.SANS2DTUBES()
        ii.MaskFile(file_name = 'USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt')

        # Setup detector positions
        ii.SetDetectorOffsets(bank = 'REAR', x = -16.0, y = 58.0, z = 0.0, rot = 0.0, radius = 0.0, side = 0.0)
        ii.SetDetectorOffsets(bank = 'FRONT', x = -44.0, y = -20.0, z = 47.0, rot = 0.0, radius = 1.0, side = 1.0)
        ii.Gravity(False)

        # Set the front detector fitting
        ii.SetFrontDetRescaleShift(scale = 1.0,shift = 0.0, fitScale = True, fitShift = True)
        ii.Set1D()

        # Assign data
        ii.AssignSample(r'SANS2D00028797.nxs', reload = True)
        ii.AssignCan(r'SANS2D00028793.nxs', reload = True)
        ii.TransmissionSample(r'SANS2D00028808.nxs', r'SANS2D00028784.nxs')
        ii.TransmissionCan(r'SANS2D00028823.nxs', r'SANS2D00028784.nxs')

        # Run the reduction and request FRONT and BACK to be merged
        ii.WavRangeReduction(combineDet="merged")

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        self.tolerance = 1e-07
        return '28797merged_1D_1.75_16.5', 'SANS2DTUBES_Merged_Reduction.nxs'

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)


class SANSMergedDetectorsTest_V2(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)

    def runTest(self):
        # Select instrument and user file
        ii2.UseCompatibilityMode()
        ii2.SANS2DTUBES()
        ii2.MaskFile(file_name='USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt')

        # Setup detector positions
        ii2.SetDetectorOffsets(bank='REAR', x=-16.0, y=58.0, z=0.0, rot=0.0, radius=0.0, side=0.0)
        ii2.SetDetectorOffsets(bank='FRONT', x=-44.0, y=-20.0, z=47.0, rot=0.0, radius=1.0, side=1.0)
        ii2.Gravity(False)

        # Set the front detector fitting
        ii2.SetFrontDetRescaleShift(scale=1.0, shift=0.0, fitScale=True, fitShift=True)
        ii2.Set1D()

        # Assign data
        ii2.AssignSample(r'SANS2D00028797.nxs', reload=True)
        ii2.AssignCan(r'SANS2D00028793.nxs', reload=True)
        ii2.TransmissionSample(r'SANS2D00028808.nxs', r'SANS2D00028784.nxs')
        ii2.TransmissionCan(r'SANS2D00028823.nxs', r'SANS2D00028784.nxs')

        # Run the reduction and request FRONT and BACK to be merged
        ii2.WavRangeReduction(combineDet="merged")

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        self.tolerance = 1e-07
        return '28797merged_1D_1.75_16.5', 'SANS2DTUBES_Merged_Reduction.nxs'

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
