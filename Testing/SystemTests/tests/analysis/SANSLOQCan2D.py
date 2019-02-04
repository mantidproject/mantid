# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import systemtesting
from mantid.simpleapi import *
from ISISCommandInterface import *

# Test is giving odd results on Linux, but only this 2D one.


class SANSLOQCan2D(systemtesting.MantidSystemTest):

    def runTest(self):

        LOQ()
        Set2D()
        Detector("main-detector-bank")
        MaskFile('MASK.094AA')
    # apply some small artificial shift
        SetDetectorOffsets('REAR', -1.0, 1.0, 0.0, 0.0, 0.0, 0.0)
        Gravity(True)

        AssignSample('99630.RAW')
        AssignCan('99631.RAW')

        WavRangeReduction(None, None, False)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
    #when comparing LOQ files you seem to need the following
        self.disableChecking.append('Axes')

        return '99630main_2D_2.2_10.0','SANSLOQCan2D.nxs'
