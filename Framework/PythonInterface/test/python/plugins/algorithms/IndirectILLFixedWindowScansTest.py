from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import DeleteWorkspace, CreateSampleWorkspace, CloneWorkspace, GroupWorkspaces
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService, WorkspaceGroup
import numpy as np

# IN16B data can have one wing or two. The IndirectILLFixedWindowScans algorithm calls IndirectILLReduction with
# UnmirrorOption=1, which is suitable for both cases.

class IndirectILLFixedWindowScans(unittest.TestCase):

    def setUp(self):
        # Elastic scan data with one wing - newer data, Doppler.Frequency not defined
        self._run_one_wing_elastic = 'ILL/IN16B/143718.nxs'
        # Inelastic scan data with two wings - older data, Doppler.frequency defined
        self._run_two_wings_inelastic = 'ILL/IN16B/083073.nxs'

    def tearDown(self):
        pass

    def test_multifiles(self):
        self._args['Run'] = 'ILL/IN16B/143718.nxs,ILL/IN16B/143718.nxs'
        self._args['Elastic'] = True

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")
        self.assertEqual(mtd['output'].size(), 2, "WorkspaceGroup should contain two runs")

        self._workspace_properties(mtd['red'])

    def test_efws(self):
        # idea: many elastic scan data have only one column that will be returned after integrating
        self._args['Run'] = self._run_one_wing_elastic
        self._args['Elastic'] = True

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_properties(mtd['output'])

    def test_ifws(self):
        self._args['Run'] = self._run_two_wings_inelastic
        self._args['Inelastic'] = True

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_properties(mtd['output'])

    def test_raise_ValueError(self):
        self._args['Run'] = self._run_two_wings_inelastic
        self._args['Inelastic'] = False
        try:
            alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        except ValueError:
            self.assertFalse(alg_test.isExecuted(), "IndirectILLFixedWindowScans executed")

    def _workspace_properties(self, ws):
        # After integration only one bin
        self.assertEqual(mtd['output'].getItem(0).blocksize(), 1)
        self.assertEqual(mtd['output'].getItem(0).getNumberHistograms(), 16)
        self.assertEqual(mtd['output'].size(), 1, "WorkspaceGroup should contain one workspace")
        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(ws.getItem(0), MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getItem(0).getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(ws.getItem(0).getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == "__main__":
    unittest.main()