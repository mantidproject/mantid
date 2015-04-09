#pylint: disable=invalid-name,no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *
import os

def _skip_test():
    """Helper function to determine if we run the test"""
    import platform

    # Only runs on RHEL6 at the moment
    if "Linux" not in platform.platform():
        return True
    flavour = platform.linux_distribution()[2]
    if flavour == 'Santiago': # Codename for RHEL6
        return False # Do not skip
    else:
        return True

class PG3Calibration(stresstesting.MantidStressTest):
    def cleanup(self):
        os.remove(self.saved_cal_file)

    def skipTests(self):
        return _skip_test()

    def requiredFiles(self):
        files = ["PG3_2538_event.nxs"]
        return files

    def requiredMemoryMB(self):
        """Requires 3Gb"""
        return 3000

    def runTest(self):
        # determine where to save
        savedir = os.path.abspath(os.path.curdir)

        # run the actual code
        output = CalibrateRectangularDetectors(OutputDirectory = savedir, SaveAs = 'calibration', FilterBadPulses = True,
                          GroupDetectorsBy = 'All', DiffractionFocusWorkspace = False, Binning = '0.5, -0.0004, 2.5',
                          MaxOffset=0.01, PeakPositions = '.6866,.7283,.8185,.8920,1.0758,1.2615,2.0599',
                          CrossCorrelation = False, Instrument = 'PG3', RunNumber = '2538', Extension = '_event.nxs')

        if isinstance(output, basestring):
            self.saved_cal_file = output
        else:
            raise NotImplementedError("Output from CalibrateRectangularDetectors is NOT string for calibration file name!")

        # load saved cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName=self.saved_cal_file, WorkspaceName="PG3_2538",
            MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_offsets",MaskedWorkspace="PG3_2538_mask")
        # load golden cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName="PG3_golden.cal", WorkspaceName="PG3_2538_golden",
            MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_golden_offsets",MaskedWorkspace="PG3_2538_golden_mask")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 2.0e-4
        return ('PG3_2538_offsets','PG3_2538_golden_offsets')

class PG3CCCalibration(stresstesting.MantidStressTest):
    def cleanup(self):
        os.remove(self.saved_cal_file)

    def skipTests(self):
        return _skip_test()

    def requiredFiles(self):
        files = ["PG3_2538_event.nxs"]
        return files

    def requiredMemoryMB(self):
        """Requires 3Gb"""
        return 3000

    def runTest(self):
        # determine where to save
        savedir = os.path.abspath(os.path.curdir)

        # run the actual code
        output = CalibrateRectangularDetectors(OutputDirectory = savedir, SaveAs = 'calibration', FilterBadPulses = True,
                          GroupDetectorsBy = 'All', DiffractionFocusWorkspace = False, Binning = '0.5, -0.0004, 2.5',
                          MaxOffset=0.01, PeakPositions = '0.7282933,1.261441',DetectorsPeaks = '17,6',
                          CrossCorrelation = True, Instrument = 'PG3', RunNumber = '2538', Extension = '_event.nxs')

        if isinstance(output, basestring):
            self.saved_cal_file = output
        else:
            raise NotImplementedError("Output from CalibrateRectangularDetectors is NOT string for calibration file name!")

        # load saved cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName=self.saved_cal_file, WorkspaceName="PG3_2538",
            MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_offsets",MaskedWorkspace="PG3_2538_mask")
        # load golden cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName="PG3_goldenCC.cal", WorkspaceName="PG3_2538_golden",
            MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_golden_offsets",MaskedWorkspace="PG3_2538_golden_mask")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 1.0e-4
        return ('PG3_2538_offsets','PG3_2538_golden_offsets')
