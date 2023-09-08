# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid.simpleapi import mtd
from mantid.simpleapi import InelasticEMUauReduction


class InelasticEMUauReductionAxisTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        InelasticEMUauReduction(
            SampleRuns="20985",
            SpectrumAxis="Q",
            OutputWorkspace="test",
            ConfigurationFile="emu_doctest.ini",
        )
        self.assertTrue("test_Q" in mtd, "Expected output workspace group in ADS")
        wg = mtd["test_Q"]
        index = dict([(tag, i) for i, tag in enumerate(wg.getNames())])
        ws = wg.getItem(index["test_Q_2D"])
        xv = ws.getAxis(0).extractValues()
        self.assertDelta(xv[0], -0.03, 0.01, "Unexpected minimum energy transfer value")
        self.assertDelta(xv[-1], 0.03, 0.01, "Unexpected minimum energy transfer value")
        yv = ws.getAxis(1).extractValues()
        self.assertDelta(yv[0], 0.3, 0.01, "Unexpected minimum Q value")
        self.assertDelta(yv[-1], 1.8, 0.01, "Unexpected minimum Q value")

        InelasticEMUauReduction(
            SampleRuns="20985",
            SpectrumAxis="TubeNumber",
            OutputWorkspace="test",
            ConfigurationFile="emu_doctest.ini",
        )
        self.assertTrue("test_TubeNumber" in mtd, "Expected output workspace group in ADS")
        wg = mtd["test_TubeNumber"]
        index = dict([(tag, i) for i, tag in enumerate(wg.getNames())])
        ws = wg.getItem(index["test_TubeNumber_2D"])
        xv = ws.getAxis(0).extractValues()
        self.assertDelta(xv[0], -0.03, 0.01, "Unexpected minimum energy transfer value")
        self.assertDelta(xv[-1], 0.03, 0.01, "Unexpected minimum energy transfer value")
        yv = ws.getAxis(1).extractValues()
        self.assertDelta(yv[0], 16.0, 0.01, "Unexpected minimum tube number value")
        self.assertDelta(yv[-1], 43.0, 0.01, "Unexpected minimum tube number value")


class InelasticEMUauReductionWSTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        InelasticEMUauReduction(
            SampleRuns="20985",
            SpectrumAxis="Q",
            OutputWorkspace="test",
            ConfigurationFile="emu_doctest.ini",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        return "test_Q_2D", "EMUauInelasticTestBase.nxs"
