# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import mantid
from mantid.simpleapi import *


class IndirectTransmissionMonitorTest(unittest.TestCase):
    def setUp(self):
        self._sample_workspace = "IndirectTransmissionMonitorTest_sample"
        self._can_workspace = "IndirectTransmissionMonitorTest_can"

        Load(Filename="IRS26176.RAW", OutputWorkspace=self._sample_workspace)
        Load(Filename="IRS26173.RAW", OutputWorkspace=self._can_workspace)

        self.kwargs = {}
        self.kwargs["SampleWorkspace"] = self._sample_workspace
        self.kwargs["CanWorkspace"] = self._can_workspace

    def test_basic(self):
        trans_workspace = IndirectTransmissionMonitor(**self.kwargs)

        self.assertTrue(isinstance(trans_workspace, mantid.api.WorkspaceGroup), msg="Result should be a workspace group")
        self.assertEqual(trans_workspace.size(), 3, msg="Transmission workspace group should have 3 workspaces: sample, can and transfer")

        expected_names = set()
        expected_names.add(self._sample_workspace + "_Can")
        expected_names.add(self._sample_workspace + "_Sam")
        expected_names.add(self._sample_workspace + "_Trans")
        self.assertEqual(set(trans_workspace.getNames()), expected_names)


if __name__ == "__main__":
    unittest.main()
