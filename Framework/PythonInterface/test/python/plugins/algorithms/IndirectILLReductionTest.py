# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

import os
import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup

class IndirectILLReductionTest(unittest.TestCase):

    _output_workspaces = []
    _args = {}
    _run_name = None
    _run_path = None
    _multi_run_name = None

    def setUp(self):
        self._run_name = 'ILL/IN16B/146191'
        self._multi_run_name = 'ILL/IN16B/146191,ILL/IN16B/146192'

    def tearDown(self):
        #clean up any files we made
        for ws in self._output_workspaces[1:]:
            path = os.path.join(config['defaultsave.directory'], ws.name() + '.nxs')
            if os.path.isfile(path):
                try:
                    os.remove(path)
                except IOError:
                    continue

        #reset output workspaces list
        self._output_workspaces = []

    def test_mutlifile(self):
        self._args['Run'] = self._multi_run_name + '.nxs'

        red = IndirectILLReduction(**self._args)

        self.assertTrue(isinstance(red, WorkspaceGroup), "Should be a group workspace")
        self.assertEqual(red.size(), 2)
        self.assertTrue(isinstance(red.getItem(0), MatrixWorkspace), "Should be a matrix workspace")


    def test_sumruns(self):
        self._args['Run'] = self._multi_run_name + '.nxs'
        self._args['SumRuns'] = True

        red = IndirectILLReduction(**self._args)
        self.assertEquals(red.size(), 1)

    def test_save_results(self):
        self._args['Run'] = self._run_name + '.nxs'
        self._args['Save'] = True

        IndirectILLReduction(**self._args)

        path = os.path.join(config['defaultsave.directory'], '146191_red.nxs')
        self.assertTrue(os.path.isfile(path), path)

    def test_no_verbose(self):
        self._args['Run'] = self._run_name + '.nxs'

        IndirectILLReduction(**self._args)

        self.assertTrue(isinstance(mtd['146191_red'], MatrixWorkspace), "Should be a matrix workspace")

    def test_debug_mode(self):
        self._args['Run'] = self._run_name + '.nxs'
        self._args['DebugMode'] = True

        IndirectILLReduction(**self._args)

        self.assertTrue(isinstance(mtd['red'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_raw'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_left'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_right'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_monitor'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_mnorm'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_vnorm'], WorkspaceGroup), "Should be a group workspace")
        self.assertTrue(isinstance(mtd['red_detgrouped'], WorkspaceGroup), "Should be a group workspace")

    def test_mapping_file_option(self):
        self._args['Run'] = self._run_name + '.nxs'

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

        IndirectILLReduction(**self._args)

        self.assertEqual(18, mtd['146191_red'].getNumberHistograms())

if __name__ == '__main__':
    unittest.main()
