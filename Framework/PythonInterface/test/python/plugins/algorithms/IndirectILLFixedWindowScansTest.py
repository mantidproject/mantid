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
        pass

    def tearDown(self):
        pass

    def test_no_verbose(self):
        self._args['Run'] = self._run_name

        IndirectILLReduction(**self._args)

        self.assertEqual(mtd['output'].size(), 1, "WorkspaceGroup should contain one workspace")
        self._workspace_properties(mtd['red'])

    def test_multifiles(self):
        self._args['Run'] = self._multi_runs

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")
        self.assertEqual(mtd['output'].size(), 2, "WorkspaceGroup should contain two runs")

        self._workspace_properties(mtd['red'])

    def test_old_run(self):
        self._args['Run'] = self._old_run

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")
        self.assertEqual(mtd['output'].size(), 1, "WorkspaceGroup should contain one workspace")

        self._workspace_properties(mtd['output'])

    def test_efws(self):
        # idea: many elastic scan data have only one column that will be returned after integrating
        pass

    def test_ifws(self):
        pass

    def test_raise_ValueError(self):
        pass

    def _workspace_properties(self, ws):
        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(ws.getItem(0), MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getItem(0).getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(ws.getItem(0).getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == "__main__":
    unittest.main()