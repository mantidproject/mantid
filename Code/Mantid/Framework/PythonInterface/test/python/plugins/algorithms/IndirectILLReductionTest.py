import unittest
import mantid
from mantid.simpleapi import *


class IndirectILLReductionTest(unittest.TestCase):

    def algorithm_type(self):
        return IndirectILLReductionTest

    def setUp(self):
        run_name = 'ILLIN16B_034745'
        self.kwargs = {}
        self.kwargs['Run'] = run_name + '.nxs'
        self.kwargs['Analyser'] = 'silicon'
        self.kwargs['Reflection'] = '111'
        self.kwargs['RawWorkspace'] = run_name + '_' + self.kwargs['Analyser'] + self.kwargs['Reflection'] + '_raw'
        self.kwargs['ReducedWorkspace'] = run_name + '_' + self.kwargs['Analyser'] + self.kwargs['Reflection'] + '_red'
        self.kwargs['Verbose'] = True

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


if __name__ == '__main__':
    unittest.main()
