# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import CompareWorkspaces, ConvertUnits, FlatPlatePaalmanPingsCorrection, LoadNexus, LoadNexusProcessed


class FlatPlatePaalmanPingsCorrectionTest(systemtesting.MantidSystemTest):
    def create_input_workspace(self, ws):
        # ws = ws with first two channels set to 2.602 AA and last two channels to 31.974AA
        LoadNexus(Filename="IN16B_felo_002K.nxs", OutputWorkspace=ws)
        ConvertUnits(InputWorkspace=ws, Target="Wavelength", EMode="Indirect", EFixed=2.08, OutputWorkspace=ws)
        ws = mtd[ws]
        val = ws.extractX()
        val[:, 0:2] = 2.602
        val[:, -2:] = 31.974
        for i in range(18):
            ws.setX(i, val[i, :])

    def do_FlatPlatePaalmanPingsTest(self, ws, ws_can, mode, name, sample_thickness=0.2, can_front_thickness=0.05, can_back_thickness=0.05):
        """
        The output workspaces in the system tests were verified by Miguel Gonzalez gonzalezm@ill.fr.

        Tested against the implementation described in:
          - J. Wuttke: 'Absorption-Correction Factors for Scattering from Flat or Tubular Samples:
            Open-Source Implementation libabsco, and Why it Should be Used with Caution',
            http://apps.jcns.fz-juelich.de/doku/sc/_media/abs00.pdf

        :param ws: The sample workspace
        :param ws_can: The can workspace
        :param mode: Direct, Indirect, Elastic or Efixed
        :return: Nothing
        """
        FPPP_Result = FlatPlatePaalmanPingsCorrection(
            SampleWorkspace=ws,
            Emode=mode,
            Efixed=2.08,
            SampleChemicalFormula="V",
            SampleDensity=0.0704565,
            SampleDensityType="Number Density",
            SampleThickness=sample_thickness,
            SampleAngle=-45,
            CanWorkspace=ws_can,
            CanChemicalFormula="Ti",
            CanDensity=0.0567,
            CanDensityType="Number Density",
            CanFrontThickness=can_front_thickness,
            CanBackThickness=can_back_thickness,
        )

        LoadNexusProcessed(Filename="FlatPlatePaalmanPings_" + name + ".nxs", OutputWorkspace="ref")
        result = CompareWorkspaces(Workspace1=FPPP_Result, Workspace2="ref", Tolerance=1e-6, CheckInstrument=False)
        if not result[0]:
            self.assertTrue(result[0], "Mismatch in " + name + ": " + result[1].row(0)["Message"])

    def runTest(self):
        self.create_input_workspace("ws")
        self.create_input_workspace("ws_can")

        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Direct", "Direct")
        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Indirect", "Indirect")
        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Elastic", "Elastic")
        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Efixed", "Efixed")

        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Indirect", "NoSample", 0.0, 0.05, 0.05)
        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Indirect", "NoCanFront", 0.2, 0.0, 0.05)
        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Indirect", "NoCanBack", 0.2, 0.05, 0.0)
        self.do_FlatPlatePaalmanPingsTest("ws", "ws_can", "Indirect", "NoCan", 0.2, 0.0, 0.0)
