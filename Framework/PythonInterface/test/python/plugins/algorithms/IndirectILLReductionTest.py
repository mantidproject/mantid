#pylint:disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import os
import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup, AnalysisDataService
from mantid import config
from testhelpers import run_algorithm


class IndirectILLReductionTest(unittest.TestCase):

    _output_workspaces = []
    _args = {}
    _run_name = None
    _run_path = None
    _run = None
    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

        self._run_name = '146191'
        self._multi_runs = '146191,146192'
        self._old_run = 'ILLIN16B_034745.nxs'

        # Reference workspace after loading (comparisons using blocksize(), getNumberHistograms(), ( SampleLogs, ...))
        ws_loaded = Load(self._run_name)
        self._run = ws_loaded

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self._def_fac
        config['default.instrument'] = self._def_inst
        config['datasearch.directories'] = self._data_dirs
        self._output_workspaces = []

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
        self.assertEqual(size, mtd['red_raw'].getItem(0).blocksize())
        self.assertEqual(size / 2, mtd['red_left'].getItem(0).blocksize())
        self.assertEqual(size / 2, mtd['red_right'].getItem(0).blocksize())
        self.assertEqual(size, mtd['red_monitor'].getItem(0).blocksize())
        self.assertEqual(size, mtd['red_mnorm'].getItem(0).blocksize())
        self.assertEqual(size, mtd['red_detgrouped'].getItem(0).blocksize())

        self.assertEqual(self._run.getNumberHistograms(), mtd['red_raw'].getItem(0).getNumberHistograms())
        #compare = CompareWorkspaces(self._run, mtd['red_raw'].getItem(0), CheckInstrument=False, CheckAxes=False)
        #self.assertTrue(compare[0])

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

    def _test_unmirror_1_2_3(self):
        self._args['Run'] = '146007'

        self._args['UnmirrorOption'] = 1
        red = IndirectILLReduction(**self._args)

        self._args['UnmirrorOption'] = 2
        left = IndirectILLReduction(**self._args)

        self._args['UnmirrorOption'] = 3
        right = IndirectILLReduction(**self._args)

        summed = Plus(left.getItem(0),right.getItem(0))

        result = CompareWorkspaces(summed,red.getItem(0))

        self._assertTrue(result[0],"Unmirror 1 should be the sum of 2 and 3")

    def _test_unmirror_4_5(self):
        pass

    def _test_unmirror_6_7(self):
        self._args['Run'] = '146007'
        self._args['UnmirrorOption'] = 6

        vana6 = IndirectILLReduction(**self._args)

        self._args['VanadiumRun'] = '146007'
        self._args['UnmirrorOption'] = 7

        vana7 = IndirectILLReduction(**self._args)

        result = CompareWorkspaces(vana6.getItem(0),vana7.getItem(0))

        self._assertTrue(result[0], "Unmirror 6 should be the same as 7 if "
                                    "the same run is also defined as vanadium run")

    def _workspace_properties(self, ws):

        self.assertTrue(isinstance(ws, WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(ws.getItem(0), MatrixWorkspace), "Should be a matrix workspace")
        self.assertTrue(ws.getItem(0).getSampleDetails(), "Should have SampleLogs")
        self.assertTrue(ws.getItem(0).getHistory().lastAlgorithm(), "Should have AlgorithmsHistory")

if __name__ == '__main__':
    unittest.main()
