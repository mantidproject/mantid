from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import mtd
from testhelpers import run_algorithm
from mantid.api import WorkspaceGroup, MatrixWorkspace
from mantid import config


class IndirectILLFixedWindowScansTest(unittest.TestCase):

    _args = {}

    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    # EFWS+IFWS, one wing
    _run_one_wing_mixed = '170257,170258,171001,171003'
    _one_wing = ['[170257, 170258]', # EFWS
                 '[171001, 171003]'] # IFWS @ 2 microEV

    # EFWS+IFWS, two wings
    _run_two_wings_mixed = '083072:083077'
    _two_wings = ['[83072, 83075]', # EFWS
                  '[83073, 83076]', # IFWS @ 1.5 microEV
                  '[83074, 83077]'] # IFWS @ 3.0 microEV

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self._def_fac
        config['default.instrument'] = self._def_inst
        config['datasearch.directories'] = self._data_dirs

    def test_one_wing(self):
        self._args['Run'] = self._run_one_wing_mixed

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_group_properties(mtd['result'], 2)

        for i in range(mtd['result'].getNumberOfEntries()):
            _item = mtd['result'].getItem(i)
            self.assertEquals(_item.getRun().getLogData('ReducedRuns').value,self._one_wing[i])
            self._workspace_properties(_item, 2)

    def test_two_wings(self):
        self._args['Run'] = self._run_two_wings_mixed

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_group_properties(mtd['result'], 3)

        for i in range(mtd['result'].getNumberOfEntries()):
            _item = mtd['result'].getItem(i)
            self.assertEquals(_item.getRun().getLogData('ReducedRuns').value,self._two_wings[i])
            self._workspace_properties(_item, 2)

    def _workspace_group_properties(self, ws, n_uniqueE):

        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertEquals(ws.getNumberOfEntries(), n_uniqueE, "Group should contain "+str(n_uniqueE)+" workspaces")

    def _workspace_properties(self, ws, n_uniqueT):

        self.assertGreater(ws.extractY().all(), 0.)
        self.assertEqual(ws.getNumberHistograms(), 18)
        self.assertEqual(ws.blocksize(), n_uniqueT)
        self.assertEqual(ws.getAxis(0).getUnit().caption(), 'Temperature')
        self.assertTrue(ws.getAxis(0).isNumeric())
        self.assertEqual(ws.getAxis(1).getUnit().caption(), 'Scattering angle')
        self.assertTrue(ws.getAxis(1).isNumeric())
        self.assertTrue(isinstance(ws, MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(ws.getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == "__main__":
    unittest.main()
