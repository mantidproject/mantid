import unittest
import testhelpers

from mantid.simpleapi import *


class CutMDTest(unittest.TestCase):
    

    def setUp(self):
        # Create a workspace
        data_ws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Mark the workspace as being in HKL
        SetSpecialCoordinates(InputWorkspace=data_ws, SpecialCoordinates='HKL')
        # Add some data to the workspace
        FakeMDEventData(InputWorkspace=data_ws, PeakParams=[10000,0,0,0,1])
        self.__in_md  = data_ws
        
    def tearDown(self):
        DeleteWorkspace(self.__in_md )

    def test_exec_throws_if_not_a_hkl_workspace(self):
        test_md = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Explicitly set the coordinate system to lab Q.
        SetSpecialCoordinates(InputWorkspace=test_md, SpecialCoordinates='Q (lab frame)')
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws")
        
    def test_slice_to_original(self):
        out_md = CutMD(self.__in_md)
        equals, result = CompareMDWorkspaces(Workspace1=self.__in_md, Workspace2=out_md)
        self.assertTrue(equals, "Input and output workspaces should be identical" )
        
    def test_wrong_projection_workspace_format_wrong_column_numbers(self):
        projection = CreateEmptyTableWorkspace()
        projection.addColumn("double", "u")
        # missing other columns
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=self.__in_md, Projection=projection, OutputWorkspace="out_ws")
        
    def test_wrong_table_workspace_format_wrong_row_numbers(self):
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("double", "w")
        projection.addColumn("double", "offset")
        # Incorrect number of rows i.e. zero in this case as none added.
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=self.__in_md, Projection=projection, OutputWorkspace="out_ws")
        
    def test_run_it(self):
        pass

if __name__ == '__main__':
    unittest.main()