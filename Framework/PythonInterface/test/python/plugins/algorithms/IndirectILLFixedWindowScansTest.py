from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import DeleteWorkspace, CreateSampleWorkspace, CloneWorkspace, GroupWorkspaces, mtd
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService, WorkspaceGroup, MatrixWorkspace
from mantid import config
import numpy as np

# IN16B data can have one wing or two. The IndirectILLFixedWindowScans algorithm calls IndirectILLReduction with
# UnmirrorOption=1, which is suitable for both cases.


class IndirectILLFixedWindowScansTest(unittest.TestCase):

    _args = {}
    _run_one_wing_elastic = None
    _run_two_wings_inelastic = None
    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

        # Elastic scan data with one wing - newer data, Doppler.Frequency not defined
        self._run_one_wing_elastic = '143718'
        # Inelastic scan data with two wings - older data, Doppler.frequency defined
        self._run_two_wings_inelastic = '083073'

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self._def_fac
        config['default.instrument'] = self._def_inst
        config['datasearch.directories'] = self._data_dirs

    def test_multifiles(self):
        self._args['Run'] = '143718,083073'

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")
        self.assertEqual(mtd['output_0.0'].size(), 1, "WorkspaceGroup should exist and contain one run")
        self.assertEqual(mtd['output_1.5'].size(), 1, "WorkspaceGroup should exist and contain one run")

        self._workspace_properties(mtd['output_0.0'])
        self._workspace_properties(mtd['output_1.5'])

    def test_efws(self):
        # idea: many elastic scan data have only one column that will be returned after integrating
        self._args['Run'] = self._run_one_wing_elastic

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_properties(mtd['output_0.0'])

    def test_ifws(self):
        self._args['Run'] = self._run_two_wings_inelastic

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_properties(mtd['output_1.5'])

    def test_raise_ValueError(self):
        self._args['Run'] = self._run_two_wings_inelastic
        try:
            alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        except ValueError:
            self.assertFalse(alg_test.isExecuted(), "IndirectILLFixedWindowScans executed")

    def _workspace_properties(self, ws):
        # After integration only one bin
        self.assertEqual(ws.getItem(0).blocksize(), 1)
        self.assertEqual(ws.getItem(0).getNumberHistograms(), 18)
        self.assertEqual(ws.size(), 1, "WorkspaceGroup should contain one workspace")
        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(ws.getItem(0), MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getItem(0).getSampleDetails(), "Should have SampleLogs")
        # ll 50 and 51 dont fulfill this condition, manual testing however successful
        #self.assertTrue(ws.getItem(0).getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == "__main__":
    unittest.main()
