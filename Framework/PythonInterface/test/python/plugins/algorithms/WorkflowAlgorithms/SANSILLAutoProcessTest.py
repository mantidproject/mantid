# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import SANSILLAutoProcess, config, mtd
from mantid.api import WorkspaceGroup, MatrixWorkspace


class SANSILLAutoProcessTest(unittest.TestCase):

    def setUp(self):
        config.appendDataSearchSubDir("ILL/D11/")
        self._facility = config["default.facility"]
        self._instrument = config["default.instrument"]
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"

    def tearDown(self):
        config["default.facility"] = self._facility
        config["default.instrument"] = self._instrument
        mtd.clear()

    def test_minimal(self):
        ws = SANSILLAutoProcess(SampleRuns="010462")

        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), 1)
        item = ws.getItem(0)
        self.assertTrue(isinstance(item, MatrixWorkspace))
        self.assertEqual(item.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEquals(item.getNumberHistograms(), 1)


if __name__ == "__main__":
    unittest.main()
