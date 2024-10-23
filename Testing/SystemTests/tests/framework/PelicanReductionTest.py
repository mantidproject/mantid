# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid.api import mtd, IEventWorkspace
from mantid.simpleapi import PelicanReduction


class PelicanReductionSOFQWTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        PelicanReduction(
            "44464",
            OutputWorkspace="test",
            EnergyTransfer="-2,0.05,2",
            MomentumTransfer="0,0.05,2",
            Processing="SOFQW1-Centre",
            ConfigurationFile="pelican_doctest.ini",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        return "test_qw1_2D", "PelicanReductionExampleSOFQW.nxs"


class PelicanReductionNXSPETest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        PelicanReduction(
            "44464",
            OutputWorkspace="test",
            EnergyTransfer="-2,0.05,2",
            MomentumTransfer="0,0.05,2",
            Processing="NXSPE",
            ConfigurationFile="pelican_doctest.ini",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        return "test_spe_2D", "PelicanReductionExampleNXSPE.nxs"


class PelicanReductionAutoQTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        PelicanReduction(
            "44464", OutputWorkspace="test", EnergyTransfer="-2,0.05,2", Processing="SOFQW1-Centre", ConfigurationFile="pelican_doctest.ini"
        )
        self.assertTrue("test_qw1" in mtd, "Expected output workspace group in ADS")
        wg = mtd["test_qw1"]
        index = dict([(tag, i) for i, tag in enumerate(wg.getNames())])
        ws = wg.getItem(index["test_qw1_2D"])
        xv = ws.dataX(0)
        self.assertDelta(xv[0], 0.0, 0.01, "Unexpected minimum Q value")
        self.assertDelta(xv[-1], 2.7, 0.01, "Unexpected maximum Q value")

    def validate(self):
        return True


class PelicanReductionDebugFilesTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        PelicanReduction(
            "44464",
            OutputWorkspace="test",
            EnergyTransfer="-2,0.05,2",
            Processing="SOFQW1-Centre",
            KeepIntermediateWorkspaces=True,
            ConfigurationFile="pelican_doctest.ini",
        )
        self.assertTrue("intermediate" in mtd, "Expected intermediate workgroup in ADS")
        self.assertTrue("_sample_merged" in mtd, "Expected merged workspace in ADS")
        ws = mtd["_sample_merged"]
        self.assertTrue(isinstance(ws, IEventWorkspace), "Expected the merged sample is an EventWorkspace")

    def validate(self):
        return True
