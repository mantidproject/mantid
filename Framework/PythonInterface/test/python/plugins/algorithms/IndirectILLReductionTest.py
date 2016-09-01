#pylint:disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import os
import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup, AnalysisDataService
from testhelpers import run_algorithm

class IndirectILLReductionTest(unittest.TestCase):

    _output_workspaces = []
    _args = {}
    _run_name = None
    _run_path = None
    _run = None

    @classmethod
    def setUp(cls):
        cls._run_name = 'ILL/IN16B/146191.nxs'
        cls._multi_runs = 'ILL/IN16B/146191.nxs,ILL/IN16B/146192.nxs'

        cls._old_run = 'ILLIN16B_034745.nxs'

        # Reference workspace after loading (comparisons using blocksize(), getNumberHistograms(), ( SampleLogs, ...))
        ws_loaded = Load(cls._run_name)
        cls._run = ws_loaded

    @classmethod
    def tearDown(cls):
        #clean up any files we made
        for ws in cls._output_workspaces[1:]:
            path = os.path.join(config['defaultsave.directory'], ws.name() + '.nxs')
            if os.path.isfile(path):
                try:
                    os.remove(path)
                except IOError:
                    continue

        if os.path.isfile('red.nxs'):
            try:
                os.remove('red.nxs')
            except IOError:
                pass

        #reset output workspaces list
        cls._output_workspaces = []

    def test_multifiles(self):
        self._args['Run'] = self._multi_runs

        alg_test = run_algorithm('IndirectILLReduction', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction not executed")
        self.assertEqual(mtd['red'].size(), 2, "WorkspaceGroup red should contain two runs")

        self._workspace_properties(mtd['red'])

    def test_sumruns(self):
        self._args['Run'] = self._multi_runs
        self._args['SumRuns'] = True

        alg_test = run_algorithm('IndirectILLReduction', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction not executed")
        self.assertEqual(mtd['red'].size(), 1, "WorkspaceGroup red should contain one workspace")

        self._workspace_properties(mtd['red'])

    def test_save_results(self):
        self._args['Run'] = self._run_name
        self._args['Save'] = True

        alg_test = run_algorithm('IndirectILLReduction', **self._args)
        path = os.path.join(config['defaultsave.directory'], 'red.nxs')

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction not executed")
        self.assertTrue(os.path.isfile(path), path)

    def test_no_verbose(self):
        self._args['Run'] = self._run_name

        IndirectILLReduction(**self._args)

        self.assertEqual(mtd['red'].size(), 1, "WorkspaceGroup red should contain one workspace")
        self._workspace_properties(mtd['red'])

    def test_old_run(self):
        self._args['Run'] = self._old_run
        self._args['UnmirrorOption'] = 0

        alg_test = run_algorithm('IndirectILLReduction', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction not executed")
        self.assertEqual(mtd['red'].size(), 1, "WorkspaceGroup red should contain one workspace")
        self._workspace_properties(mtd['red'])

    def test_debug_mode(self):
        self._args['Run'] = self._run_name
        self._args['DebugMode'] = True

        alg_test = run_algorithm('IndirectILLReduction', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction failed")

        self._workspace_properties(mtd['red'])
        self._workspace_properties(mtd['red_raw'])
        self._workspace_properties(mtd['red_left'])
        self._workspace_properties(mtd['red_right'])
        self._workspace_properties(mtd['red_monitor'])
        self._workspace_properties(mtd['red_mnorm'])
        self._workspace_properties(mtd['red_detgrouped'])

        size = self._run.blocksize()

        # Further workspace characteristics
        self.assertEqual(size / 2, mtd['red'].getItem(0).blocksize())
        self.assertEqual(size    , mtd['red_raw'].getItem(0).blocksize())
        self.assertEqual(size / 2, mtd['red_left'].getItem(0).blocksize())
        self.assertEqual(size / 2, mtd['red_right'].getItem(0).blocksize())
        self.assertEqual(size    , mtd['red_monitor'].getItem(0).blocksize())
        self.assertEqual(size    , mtd['red_mnorm'].getItem(0).blocksize())
        self.assertEqual(size    , mtd['red_detgrouped'].getItem(0).blocksize())

        self.assertEqual(self._run.getNumberHistograms(), mtd['red_raw'].getItem(0).getNumberHistograms())
        self.assertEqual("Success!", CheckWorkspacesMatch(self._run, mtd['red_raw'].getItem(0)))

    def test_debug_calibration(self):

        self._args['Run'] = self._run_name
        self._args['DebugMode'] = True
        self._args['CalibrationWorkspace'] = ILLIN16BCalibration(self._run_name)

        alg_test = run_algorithm('IndirectILLReduction', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction failed")
        self._workspace_properties(mtd['red_vnorm'])
        self.assertEqual(self._run.blocksize(), mtd['red_vnorm'].getItem(0).blocksize())

    def test_mapping_file_option(self):
        self._args['Run'] = self._run_name

        # manually get name of grouping file from parameter file
        idf = os.path.join(config['instrumentDefinition.directory'], "IN16B_Definition.xml")
        ipf = os.path.join(config['instrumentDefinition.directory'], "IN16B_Parameters.xml")

        ws = LoadEmptyInstrument(Filename=idf)
        LoadParameterFile(ws, Filename=ipf)
        instrument = ws.getInstrument()
        grouping_filename = instrument.getStringParameter('Workflow.GroupingFile')[0]
        DeleteWorkspace(ws)

        #supply grouping file as option to algorithm, this tests that a different file can be used
        #rather than reading the default directly from the IP File.
        self._args['MapFile'] = os.path.join(config['groupingFiles.directory'], grouping_filename)

        alg_test = run_algorithm('IndirectILLReduction', **self._args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReduction not executed")
        self._workspace_properties(mtd['red'])

    def _workspace_properties(self, ws):

        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(ws.getItem(0), MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getItem(0).getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(ws.getItem(0).getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == '__main__':
    unittest.main()
