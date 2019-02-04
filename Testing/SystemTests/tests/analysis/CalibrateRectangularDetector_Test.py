# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,no-init,attribute-defined-outside-init
from __future__ import (absolute_import, division, print_function)
import systemtesting
import os
from mantid.simpleapi import *
from six import string_types


def _skip_test():
    """Helper function to determine if we run the test"""
    import platform

    # don't run on rhel6
    if "redhat-6" in platform.platform():
        return True
    # run on any other linux
    return "Linux" not in platform.platform()


class PG3Calibration(systemtesting.MantidSystemTest):
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
                                               GroupDetectorsBy = 'All', DiffractionFocusWorkspace = True,
                                               Binning = '0.5, -0.0004, 2.5',
                                               MaxOffset=0.01, PeakPositions = '.6866,.7283,.8185,.8920,1.0758,1.2615,2.0599',
                                               CrossCorrelation = False, RunNumber = 'PG3_2538')

        if isinstance(output, string_types):
            self.saved_cal_file = output.replace('.h5','.cal')
        else:
            raise NotImplementedError("Output from CalibrateRectangularDetectors is NOT string for calibration file name!")

        # load saved cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName=self.saved_cal_file,
                    WorkspaceName="PG3_2538", MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_offsets",MaskedWorkspace="PG3_2538_mask")
        # load golden cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName="PG3_golden.cal",
                    WorkspaceName="PG3_2538_golden", MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_golden_offsets",MaskedWorkspace="PG3_2538_golden_mask")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 2.0e-4
        return ('PG3_2538_offsets','PG3_2538_golden_offsets')


class PG3CCCalibration(systemtesting.MantidSystemTest):
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
                                               GroupDetectorsBy = 'All', DiffractionFocusWorkspace = False,
                                               Binning = '0.5, -0.0004, 2.5',
                                               MaxOffset=0.01, PeakPositions = '0.7282933,1.261441',DetectorsPeaks = '17,6',
                                               CrossCorrelation = True, RunNumber = 'PG3_2538')

        if isinstance(output, string_types):
            self.saved_cal_file = output.replace('.h5','.cal')
        else:
            raise NotImplementedError("Output from CalibrateRectangularDetectors is NOT string for calibration file name!")

        # load saved cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName=self.saved_cal_file,
                    WorkspaceName="PG3_2538", MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_offsets",MaskedWorkspace="PG3_2538_mask")
        MaskBTP(Workspace="PG3_2538_offsets", Pixel="0,6")
        MaskBTP(Workspace="PG3_2538_offsets",Tube="0-24,129-153")
        # load golden cal file
        LoadCalFile(InputWorkspace="PG3_2538_calibrated", CalFileName="PG3_goldenCC.cal",
                    WorkspaceName="PG3_2538_golden", MakeGroupingWorkspace=False)
        MaskDetectors(Workspace="PG3_2538_golden_offsets",MaskedWorkspace="PG3_2538_golden_mask")
        MaskBTP(Workspace="PG3_2538_golden_offsets",Pixel="0,6")
        MaskBTP(Workspace="PG3_2538_golden_offsets",Tube="0-24,129-153")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 1.0e-4
        return ('PG3_2538_offsets','PG3_2538_golden_offsets')
