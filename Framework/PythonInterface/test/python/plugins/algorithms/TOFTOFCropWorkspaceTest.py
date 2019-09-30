# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import Load, DeleteWorkspace, GroupWorkspaces, TOFTOFCropWorkspace
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService


class TOFTOFCropWorkspaceTest(unittest.TestCase):

    _input_ws = None
    _cropped_ws = None

    def setUp(self):
        input_ws = Load(Filename="TOFTOFTestdata.nxs")
        self._input_ws = input_ws

    def test_basicrun(self):
        OutputWorkspaceName = "cropped_ws"
        alg_test = run_algorithm("TOFTOFCropWorkspace",
                                 InputWorkspace=self._input_ws,
                                 OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())
        self._cropped_ws = AnalysisDataService.retrieve(OutputWorkspaceName)

        run = self._cropped_ws.getRun()
        # check existence of required entries in logs
        self.assertTrue('full_channels' in run.keys())
        self.assertTrue('channel_width' in run.keys())
        # check their values
        full_channels = float(run.getLogData('full_channels').value)
        channel_width = float(run.getLogData('channel_width').value)
        self.assertGreater(full_channels, 0.)
        self.assertGreater(channel_width, 0.)
        # check unit horizontal axis
        self.assertEqual(self._cropped_ws.getAxis(0).getUnit().unitID(), 'TOF')
        # check length of cropped ws
        self.assertEqual(len(self._cropped_ws.readX(0)), int(full_channels))

    def test_inputgroup(self):
        group = GroupWorkspaces([self._input_ws])
        OutputWorkspaceName = "cropped_ws"
        alg_test = run_algorithm("TOFTOFCropWorkspace",
                                 InputWorkspace=group,
                                 OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())

    def test_invalid_xunits(self):
        self._input_ws.getAxis(0).setUnit('Wavelength')
        OutputWorkspaceName = "cropped_ws"
        self.assertRaises(RuntimeError, TOFTOFCropWorkspace, InputWorkspace=self._input_ws,
                          OutputWorkspace=OutputWorkspaceName)

    def cleanUp(self):
        if AnalysisDataService.doesExist(self._input_ws):
            DeleteWorkspace(self._input_ws)
        if AnalysisDataService.doesExist(self._cropped_ws):
            DeleteWorkspace(self._cropped_ws)

if __name__ == "__main__":
    unittest.main()
