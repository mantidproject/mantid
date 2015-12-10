#pylint: disable=invalid-name

from mantid.simpleapi import *
import ISISCommandInterface as i
import stresstesting
class SANSMergedDetectorsTest(stresstesting.MantidStressTest):
    def runTest(self):

        i.SANS2DTUBES()
        i.MaskFile('USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt')
        i.SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        i.SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        i.Gravity(False)
        # Set the front detector fitting
        i.SetFrontDetRescaleShift(scale=1.0,shift=0.0, fitScale=True, fitShift=True)
        i.Set1D()
        i.AssignSample(r'SANS2D00028797.nxs', reload = True)
        i.AssignCan(r'SANS2D00028793.nxs', reload = True)
        i.TransmissionSample(r'SANS2D00028808.nxs', r'SANS2D00028784.nxs')
        i.TransmissionCan(r'SANS2D00028823.nxs', r'SANS2D00028784.nxs')
        # Request a merged reduction
        i.WavRangeReduction(combineDet="merged")

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance = 0.01
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '28797merged_1D_1.75_16.5', 'SANS2DTUBES_Merged_Reduction.nxs'

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
