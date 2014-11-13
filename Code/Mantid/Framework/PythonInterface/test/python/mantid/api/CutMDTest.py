import unittest
import testhelpers

from mantid.simpleapi import *
from mantid.api import IMDHistoWorkspace


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
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1])
        
    def test_slice_to_original(self):
        out_md = CutMD(self.__in_md, P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1])
        self.assertTrue(isinstance(out_md, IMDHistoWorkspace))
        # No rotation. Basis vectors should have been left the same, so no extent changes.
        self.assertEquals(self.__in_md.getDimension(0).getMinimum(), out_md.getDimension(0).getMinimum())
        self.assertEquals(self.__in_md.getDimension(0).getMaximum(), out_md.getDimension(0).getMaximum())
        self.assertEquals(self.__in_md.getDimension(1).getMinimum(), out_md.getDimension(1).getMinimum())
        self.assertEquals(self.__in_md.getDimension(1).getMaximum(), out_md.getDimension(1).getMaximum())
        self.assertEquals(self.__in_md.getDimension(2).getMinimum(), out_md.getDimension(2).getMinimum())
        self.assertEquals(self.__in_md.getDimension(2).getMaximum(), out_md.getDimension(2).getMaximum())
        self.assertEquals("['zeta', 0.0, 0.0]",  out_md.getDimension(0).getName() )
        self.assertEquals("[0.0, 'eta', 0.0]",  out_md.getDimension(1).getName() )
        self.assertEquals("[0.0, 0.0, 'xi']",  out_md.getDimension(2).getName() )
        
    def test_wrong_projection_workspace_format_wrong_column_numbers(self):
        projection = CreateEmptyTableWorkspace()
        projection.addColumn("double", "u")
        # missing other columns
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=self.__in_md, Projection=projection, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1])
        
    def test_wrong_table_workspace_format_wrong_row_numbers(self):
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("double", "w")
        projection.addColumn("double", "offset")
        projection.addColumn("string", "type")
        # Incorrect number of rows i.e. zero in this case as none added.
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=self.__in_md, Projection=projection, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1])
        
    def test_run_it(self):
        pass

if __name__ == '__main__':
    unittest.main()