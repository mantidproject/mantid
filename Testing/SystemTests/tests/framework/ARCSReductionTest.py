# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
System test for ARCS reduction
"""

import os
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import DeleteWorkspace, DgsReduction, ExtractMask, Load, LoadNXSPE, SaveNXSPE


class ARCSReductionTest(systemtesting.MantidSystemTest):
    vanFile1 = ""
    vanFile0 = ""
    nxspeFile = ""

    def skipTests(self):
        """
        This test relies on comparing 0.0 in the result to NaN in the reference in several bins.
        This used to evaluate as true due to a bug.  The bug fixed in PR #38075, causing this test to fail.
        The problem is likely due to the older behavior of bin masking in use when the reference was made.
        To be fixed as part of Issue #38088
        """
        return True

    def requiredFiles(self):
        return ["ARCS_23961_event.nxs", "WBARCS.nxs"]

    def requiredMemoryMB(self):
        return 4000

    def cleanup(self):
        if os.path.exists(self.nxspeFile):
            os.remove(self.nxspeFile)
        if os.path.exists(self.vanFile1):
            os.remove(self.vanFile1)
        if os.path.exists(self.vanFile0):
            os.remove(self.vanFile0)
        return True

    def runTest(self):
        self.vanFile1 = os.path.join(config.getString("defaultsave.directory"), "ARCSvan_1.nxs")
        self.vanFile0 = os.path.join(config.getString("defaultsave.directory"), "ARCSvan_0.nxs")
        self.nxspeFile = os.path.join(config.getString("defaultsave.directory"), "ARCSsystemtest.nxspe")
        config["default.facility"] = "SNS"
        DgsReduction(
            SampleInputFile="ARCS_23961_event.nxs",
            OutputWorkspace="reduced",
            IncidentBeamNormalisation="ByCurrent",
            DetectorVanadiumInputFile="WBARCS.nxs",
            UseBoundsForDetVan=True,
            DetVanIntRangeLow=0.35,
            DetVanIntRangeHigh=0.75,
            DetVanIntRangeUnits="Wavelength",
            SaveProcessedDetVan=True,
            SaveProcDetVanFilename=self.vanFile0,
        )
        DgsReduction(
            SampleInputFile="ARCS_23961_event.nxs",
            OutputWorkspace="reduced",
            IncidentBeamNormalisation="ByCurrent",
            DetectorVanadiumInputFile="WBARCS.nxs",
            UseBoundsForDetVan=True,
            DetVanIntRangeLow=0.35,
            DetVanIntRangeHigh=0.75,
            DetVanIntRangeUnits="Wavelength",
            MedianTestLevelsUp=1.0,
            SaveProcessedDetVan=True,
            SaveProcDetVanFilename=self.vanFile1,
        )

        Ei = mtd["reduced"].run().get("Ei").value
        SaveNXSPE(InputWorkspace="reduced", Filename=self.nxspeFile, Efixed=Ei, psi=0, KiOverKfScaling=True)

    def validate(self):
        # test vanadium file
        self.assertTrue(os.path.exists(self.vanFile0))
        self.assertTrue(os.path.exists(self.vanFile1))
        van0 = Load(self.vanFile0)
        van1 = Load(self.vanFile1)
        m0 = ExtractMask(van0)
        m1 = ExtractMask(van1)
        self.assertGreaterThan(len(m0[1]), len(m1[1]))  # levelsUp=1 should have less pixels masked
        DeleteWorkspace("m0")
        DeleteWorkspace("m1")
        DeleteWorkspace(van0)
        DeleteWorkspace(van1)
        self.assertTrue(os.path.exists(self.nxspeFile))
        LoadNXSPE(self.nxspeFile, OutputWorkspace="nxspe")
        self.disableChecking.append("Instrument")

        return "nxspe", "ARCSsystemtest.nxs"
        pass
