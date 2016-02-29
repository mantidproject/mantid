import unittest
import platform
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup

class IndirectNormSpectraTest(unittest.TestCase):

    def setUp(self):

    def tearDown(self):

#----------------------------------Algorithm tests----------------------------------------

    def test_with_MatrixWorkspace_one_hist(self):

    def test_with_MatrixWorkspace_multi_hist(self):
    
    def test_with_WorkspaceGroup_one_workspace(self):
    
    def test_with_WorkspaceGroup_multi_workspace(self):

#----------------------------------Failure cases------------------------------------------

    def test_with_MatrixWorkspace_with_zero_histograms(self):
    
    def test_with_TableWorkspace(self):
    
#--------------------------------Validate results-----------------------------------------
    def _check_MatrixWorkspace_for_y_more_than_one(self, matrixWs):
        """
        Checks if any of the y values in the matrix workspace are more than 1
        """
        for i in range(matrixWs.getNumberHistograms()):
            self._check_spectrum_for_y_more_than_one(matrixWs.readY(i))

    def _check_spectrum_for_y_more_than_one(self, y_data):
        for i in range(len(y_data)):
            self.assertLessEqual(y_data[i], 1)
    
#--------------------------------Helper Functions-----------------------------------------
    def _create_MatrixWorkspace(self, nhists, name):
        """
        Creates a basic Matrixworkspace with nHists
        @param nhists :: The numebr of histograms
        @param name :: The name of the MatrixWorkspace
        """
        data = []
        for i in range(nhists):
            data.append[1,2,3,4,5]
        CreateWorkspace(OutputWorkspace=name, DataX=data, DataY=data, DataE=data, Nspec=nhists)
        return mtd[name]

    def _create_WorkspaceGroup(self, items, name):
        """
        Creates a WorkspaceGroup with n items
        @parmas items :: The number of entries in the WorkspaceGroup
        @params name  :: The base name of the Workspaces
        """
        workspaces = []
        for i in range(items):
            ws = self._create_MatrixWorkspace(items, (name + str(i)))
            workspaces.append(ws.getName())
        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=name+'_group')
        return mtd[name + '_group']

if __name__=="__main__":
    unittest.main()
