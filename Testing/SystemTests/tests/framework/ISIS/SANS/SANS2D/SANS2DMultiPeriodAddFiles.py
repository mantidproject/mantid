# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid import config
from ISISCommandInterface import *
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DMultiPeriodAddFiles(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2Doptions.091A")
        Gravity(True)
        add_runs(("5512", "5512"), "SANS2D", "nxs", lowMem=True)

        # one period of a multi-period Nexus file
        AssignSample("5512-add.nxs", period=7)

        WavRangeReduction(2, 4, DefaultTrans)

        paths = [
            os.path.join(config["defaultsave.directory"], "SANS2D00005512-add.nxs"),
            os.path.join(config["defaultsave.directory"], "SANS2D00005512.log"),
        ]
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Axes")

        return "5512p7rear_1D_2.0_4.0Phi-45.0_45.0", "SANS2DMultiPeriodAddFiles.nxs"
