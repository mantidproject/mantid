import unittest
import platform
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup

class IndirectNormSpectraTest(unittest.TestCase):

    def setUp(self):

    def tearDown(self):

#----------------------------------Algorithm tests----------------------------------------

    def test_with_MatrixWorkspace_one_hist(self):
        in_ws = self._create_MatrixWorkspace(1, test)
        out_ws = IndirectNormSpectra(InputWorkspace=in_ws)
        self._check_MatrixWorkspace_for_y_more_than_one(out_ws)

    def test_with_MatrixWorkspace_multi_hist(self):
        in_ws = self._create_MatrixWorkspace(3, test)
        out_ws = IndirectNormSpectra(InputWorkspace=in_ws)
        self._check_MatrixWorkspace_for_y_more_than_one(out_ws)
    
    def test_with_WorkspaceGroup_one_workspace(self):
        in_ws = self._create_WorkspaceGroup(1, test)
        out_ws = IndirectNormSpectra(InputWorkspace=in_ws)
        self._check_MatrixWorkspace_for_y_more_than_one(out_ws)
    
    def test_with_WorkspaceGroup_multi_workspace(self):
        in_ws = self._create_WorkspaceGroup(1, test)
        out_ws = IndirectNormSpectra(InputWorkspace=in_ws)
        self._check_MatrixWorkspace_for_y_more_than_one(out_ws)

#----------------------------------Failure cases------------------------------------------

    def test_with_MatrixWorkspace_with_zero_histograms(self):
    
    def test_with_TableWorkspace(self):
    
#--------------------------------Validate results-----------------------------------------
    def _check_all_items_in_workspace_group(self, workspace_group):
        items = workspace_group.getNumberOfEntries
        for i in range(items):
            self._check_MatrixWorkspace_for_y_more_than_one(workspace_group.getItem(i))

    def _check_MatrixWorkspace_for_y_more_than_one(self, matrixWs):
        for i in range(matrixWs.getNumberHistograms()):
            self._check_spectrum_for_y_more_than_one(matrixWs.readY(i))

    def _check_spectrum_for_y_more_than_one(self, y_data):
        for i in range(len(y_data)):
            self.assertLessEqual(y_data[i], 1)
    
#--------------------------------Helper Functions-----------------------------------------
    def _create_MatrixWorkspace(self, nhists, out_name):
        """
        Creates a basic Matrixworkspace
        """
        data = []
        for i in range(nhists):
            data.append[1,2,3,4,5]
        CreateWorkspace(OutputWorkspace=out_name, DataX=data, DataY=data, DataE=data, Nspec=nhists)
        return mtd[out_name]

    def _create_WorkspaceGroup(self, items, out_name):
        """
        Creates a WorkspaceGroup with n items
        @parmas items :: The number of entries in the WorkspaceGroup
        @params name  :: The base name of the Workspaces
        """
        workspaces = []
        for i in range(items):
            ws = self._create_MatrixWorkspace(items, (name + str(i)))
            workspaces.append(ws.getName())
        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=out_name+'_group')
        return mtd[out_name + '_group']

if __name__=="__main__":
    unittest.main()
