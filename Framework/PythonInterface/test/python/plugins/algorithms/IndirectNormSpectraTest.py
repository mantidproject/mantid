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
    def _check_MatrixWorkspace_for_y_more_than_1(self):
    

if __name__=="__main__":
    unittest.main()
