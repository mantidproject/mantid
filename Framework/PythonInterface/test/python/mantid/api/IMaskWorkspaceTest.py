from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import AnalysisDataService, IMaskWorkspace
from testhelpers import run_algorithm, WorkspaceCreationHelper

class IMaskWorkspaceTest(unittest.TestCase):

    def test_MaskWorkspace_Is_Retrievable(self):
        dummy_ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 102, False) # no monitors
        ws_name = "dummy"
        AnalysisDataService.add(ws_name, dummy_ws)
        run_algorithm('MaskDetectors', Workspace=ws_name, WorkspaceIndexList=1)
        mask_name = 'mask_ws'
        run_algorithm('ExtractMask', InputWorkspace=ws_name, OutputWorkspace=mask_name)
        masked_ws = AnalysisDataService[mask_name]

        self.assertTrue(isinstance(masked_ws, IMaskWorkspace))
        self.assertEqual(1, masked_ws.getNumberMasked())
        # single number
        self.assertTrue(not masked_ws.isMasked(1))
        self.assertTrue(masked_ws.isMasked(2))
        # list
        self.assertTrue(not masked_ws.isMasked([1]))
        self.assertTrue(masked_ws.isMasked([2]))


if __name__ == '__main__':
    unittest.main()
