# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting

from mantid.api import AnalysisDataService
from sans.command_interface.ISISCommandInterface import (SANS2D, Set1D, Detector, MaskFile, Gravity,
                                                         UseCompatibilityMode, AssignSample, WavRangeReduction)

# test batch mode with sans2d and selecting a period in batch mode


class SANS2DMultiPeriodSingleTest(systemtesting.MantidSystemTest):

    reduced = ''

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('Mask_SANS2D_091_options.toml')
        Gravity(True)

        AssignSample('5512')
        self.reduced = WavRangeReduction()

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return AnalysisDataService[self.reduced][6].name(),'SANS2DBatch.nxs'
