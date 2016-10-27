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

    # EFWS, one wing, newer data
    _run_one_wing_elastic = '143718'
    # IFWS, two wings, older data, Doppler.frequency defined
    _run_two_wings_inelastic = '083073'

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

    def test_efws(self):
        self._args['Run'] = self._run_one_wing_elastic

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_properties(mtd['output'])

    def test_ifws(self):
        self._args['Run'] = self._run_two_wings_inelastic

        alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLFixedWindowScans not executed")

        self._workspace_properties(mtd['output'])

    def test_raise_ValueError(self):
        self._args['Run'] = self._run_two_wings_inelastic
        try:
            alg_test = run_algorithm('IndirectILLFixedWindowScans', **self._args)
        except ValueError:
            self.assertFalse(alg_test.isExecuted(), "IndirectILLFixedWindowScans executed")

    def _workspace_properties(self, ws):
        # After integration only one bin, all entries for all spectra > 0
        self.assertGreater(ws.getItem(0).extractY().all(), 0.)
        self.assertGreater(ws.getItem(0).extractX().all(), 0.)
        # The first two spectra contain zero-valued errors (single detectors) - no test since y-values and e-values are
        # treated equivalently
        # Check log file entry if original run number was saved
        self.assertTrue(ws.getItem(0).getRun().getLogData('ReducedRuns').value)
        self.assertEqual(ws.getItem(0).blocksize(), 1)
        self.assertEqual(ws.getItem(0).getNumberHistograms(), 18)
        # Workspaces should have x- unit temperature K as default (check for symbol K  DOES NOT WORK)
        self.assertEqual(ws.getItem(0).getAxis(0).getUnit().caption(), 'Temperature')
        #self.assertEqual(ws.getItem(0).getAxis(0).getUnit().symbol(), 'K')
        self.assertTrue(ws.getItem(0).getAxis(0).isNumeric())
        # Workspace should have y- unit scattering angle as default (check for symbol degrees DOES NOT WORK)
        self.assertEqual(ws.getItem(0).getAxis(1).getUnit().caption(), 'Scattering angle')
        self.assertTrue(ws.getItem(0).getAxis(1).isNumeric())
        self.assertEqual(ws.size(), 1, "WorkspaceGroup should contain one workspace")
        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(ws.getItem(0), MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getItem(0).getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(ws.getItem(0).getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == "__main__":
    unittest.main()
