# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import DeleteWorkspace

from ISISCommandInterface import *
from sans_core.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LARMOR)
class LARMORMultiPeriodAddEventFiles(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        LARMOR()
        Set1D()
        Detector("DetectorBench")
        MaskFile("USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt")
        Gravity(True)
        add_runs(("13065", "13065"), "LARMOR", "nxs", lowMem=True)

        AssignSample("13065-add.nxs")
        WavRangeReduction(2, 4, DefaultTrans)

        # Clean up
        to_clean = [
            "13065_sans_nxs",
            "13065p1rear_1D_2.0_4.0_incident_monitor",
            "13065p2rear_1D_2.0_4.0_incident_monitor",
            "13065p3rear_1D_2.0_4.0_incident_monitor",
            "13065p4rear_1D_2.0_4.0_incident_monitor",
            "80tubeCalibration_1-05-2015_r3157-3160",
        ]
        for workspace in to_clean:
            DeleteWorkspace(workspace)

        paths = [
            os.path.join(config["defaultsave.directory"], "LARMOR00013065-add.nxs"),
            os.path.join(config["defaultsave.directory"], "SANS2D00013065.log"),
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

        return "13065p1rear_1D_2.0_4.0", "LARMORMultiPeriodAddEventFiles.nxs"
