import unittest, os
import mantid
from mantid.simpleapi import *

class IndirectILLReductionTest(unittest.TestCase):

    _output_workspaces = []

    def setUp(self):
        run_name = 'ILLIN16B_034745'
        self.kwargs = {}
        self.kwargs['Run'] = run_name + '.nxs'
        self.kwargs['Analyser'] = 'silicon'
        self.kwargs['Reflection'] = '111'
        self.kwargs['RawWorkspace'] = run_name + '_' + self.kwargs['Analyser'] + self.kwargs['Reflection'] + '_raw'
        self.kwargs['ReducedWorkspace'] = run_name + '_' + self.kwargs['Analyser'] + self.kwargs['Reflection'] + '_red'

    def tearDown(self):
        #clean up any files we made
        for ws in self._output_workspaces[1:]:
            path = os.path.join(config['defaultsave.directory'], ws.name() + '.nxs')
            if (os.path.isfile(path)):
                try:
                    os.remove(path)
                except IOError, e:
                    continue

        #reset output workspaces list
        self._output_workspaces = []

    def test_minimal(self):
        IndirectILLReduction(**self.kwargs)

        red_workspace = mtd[self.kwargs['ReducedWorkspace']]

        self.assertTrue(isinstance(red_workspace, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Energy transfer", red_workspace.getAxis(0).getUnit().caption())

    def test_mirror_mode(self):
        self.kwargs['MirrorMode'] = True
        self.kwargs['LeftWorkspace'] = self.kwargs['ReducedWorkspace'] + '_left'
        self.kwargs['RightWorkspace'] = self.kwargs['ReducedWorkspace'] + '_right'

        IndirectILLReduction(**self.kwargs)

        red_workspace = mtd[self.kwargs['ReducedWorkspace']]
        left_red_workspace = mtd[self.kwargs['LeftWorkspace']]
        right_red_workspace = mtd[self.kwargs['RightWorkspace']]

        self.assertTrue(isinstance(red_workspace, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Energy transfer", red_workspace.getAxis(0).getUnit().caption())
        self.assertEqual("meV", red_workspace.getAxis(0).getUnit().symbol().ascii())

        self.assertTrue(isinstance(left_red_workspace, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Energy transfer", left_red_workspace.getAxis(0).getUnit().caption())
        self.assertEqual("meV", left_red_workspace.getAxis(0).getUnit().symbol().ascii())

        self.assertTrue(isinstance(right_red_workspace, mantid.api.MatrixWorkspace), "Should be a matrix workspace")
        self.assertEqual("Energy transfer", right_red_workspace.getAxis(0).getUnit().caption())
        self.assertEqual("meV", right_red_workspace.getAxis(0).getUnit().symbol().ascii())

    def test_mirror_mode_default_names(self):
        self.kwargs['MirrorMode'] = True

        self.assertRaises(RuntimeError, IndirectILLReduction, **self.kwargs)

        left = self.kwargs['ReducedWorkspace'] + '_left'
        right = self.kwargs['ReducedWorkspace'] + '_right'

        self.assertTrue(mtd.doesExist(left))
        self.assertTrue(mtd.doesExist(right))

    def test_save_results(self):
        self.kwargs['Save'] = True

        self._output_workspaces = IndirectILLReduction(**self.kwargs)

        path = os.path.join(config['defaultsave.directory'], self.kwargs['ReducedWorkspace'] + '.nxs')
        self.assertTrue(os.path.isfile(path))

    def test_save_mirror_mode_output(self):
        self.kwargs['Save'] = True
        self.kwargs['MirrorMode'] = True
        self.kwargs['LeftWorkspace'] = self.kwargs['ReducedWorkspace'] + '_left'
        self.kwargs['RightWorkspace'] = self.kwargs['ReducedWorkspace'] + '_right'

        self._output_workspaces = IndirectILLReduction(**self.kwargs)

        for ws in self._output_workspaces[1:]:
            path = os.path.join(config['defaultsave.directory'], ws.name() + '.nxs')
            self.assertTrue(os.path.isfile(path))

    def test_no_verbose(self):
        IndirectILLReduction(**self.kwargs)

        red_workspace = mtd[self.kwargs['ReducedWorkspace']]
        self.assertTrue(isinstance(red_workspace, mantid.api.MatrixWorkspace), "Should be a matrix workspace")

    def test_mapping_file_option(self):
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
        self.kwargs['MapFile'] = os.path.join(config['groupingFiles.directory'], grouping_filename)

        IndirectILLReduction(**self.kwargs)
        red_workspace = mtd[self.kwargs['ReducedWorkspace']]
        self.assertEqual(24, red_workspace.getNumberHistograms())


if __name__ == '__main__':
    unittest.main()
