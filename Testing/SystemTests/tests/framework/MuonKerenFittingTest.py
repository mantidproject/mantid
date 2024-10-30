# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init,too-few-public-methods
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import Fit, Load, MuonProcess


class MuonKerenFittingTest(systemtesting.MantidSystemTest):
    """Tests the Keren fitting function on a real workspace, to check results vs. WiMDA"""

    def runTest(self):
        # Load dataset
        Load(Filename="MUT00053591.nxs", DetectorGroupingTable="gp", OutputWorkspace="MUT53591")

        # Process like MuonAnalysis interface would
        MuonProcess(
            InputWorkspace="MUT53591",
            Mode="Combined",
            SummedPeriodSet="1",
            ApplyDeadTimeCorrection=False,
            DetectorGroupingTable="gp",
            LoadedTimeZero=0,
            TimeZero=0,
            Xmin=0.08,
            Xmax=10.0,
            OutputType="PairAsymmetry",
            PairFirstIndex="0",
            PairSecondIndex="1",
            Alpha=1.0,
            OutputWorkspace="processed",
        )

        # Fit the Keren function to the data
        func = "name=FlatBackground,A0=0.1;name=Keren,A=0.1,Delta=0.2,Field=18,Fluct=0.2"
        Fit(InputWorkspace="processed", Function=func, Output="out", CreateOutput=True)

        # Get fitted parameters
        params = mtd["out_Parameters"]
        Background = params.cell(0, 1)
        Initial = params.cell(1, 1)
        Delta = params.cell(2, 1)
        Field = params.cell(3, 1)
        Fluct = params.cell(4, 1)
        Chisq = params.cell(5, 1)

        # Check that params are within the errors of those obtained in WiMDA
        self.assertTrue(Chisq < 1.1, "Fitted chi-square too large")
        self.assertDelta(Background, 0.1623, 0.0046, "Fitted A0 outside errors")
        self.assertDelta(Initial, 0.0389, 0.0040, "Fitted A outside errors")
        self.assertDelta(Delta, 0.96, 0.11, "Fitted Delta outside errors")
        self.assertDelta(Field, 20.0, 1.0, "Fitted Field outside errors")
        self.assertDelta(Fluct, 0.1, 0.01, "Fitted Fluct outside errors")
