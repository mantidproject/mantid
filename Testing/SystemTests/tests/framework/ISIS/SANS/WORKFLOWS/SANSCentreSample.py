# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
from ISISCommandInterface import *
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANSCentreSample(systemtesting.MantidSystemTest):
    def runTest(self):
        SANS2D()

        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2D.091A")

        AssignSample("992.raw")

        FindBeamCentre(60, 280, 19, 100.0 / 1000.0, -200.0 / 1000.0)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")

        return "992_sans_raw", "SANSCentreSample.nxs"
