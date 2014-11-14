import unittest
import testhelpers
import numpy as np
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
        self.assertEquals("['zeta', 0, 0]",  out_md.getDimension(0).getName() )
        self.assertEquals("[0, 'eta', 0]",  out_md.getDimension(1).getName() )
        self.assertEquals("[0, 0, 'xi']",  out_md.getDimension(2).getName() )
        
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
        
    def test_non_orthogonal_slice_with_scaling(self):
        # We create a fake workspace around and check to see that the extents get scaled with the new coordinate system when sliced
        to_cut = CreateMDWorkspace(Dimensions=3, Extents=[-1,1,-1,1,-1,1], Names='H,K,L', Units='U,U,U')
        
        SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')
        
        scale_x = 2.0
        scale_y = 2.0
                    
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("str", "type")
        projection.addRow([scale_x,0,"aaa"])
        projection.addRow([0,scale_y,"aaa"])  
        projection.addRow([0,0,"aaa"])   
                    
        out_md = CutMD(to_cut, Projection=projection, P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1])
        
        scale_z = np.cross(projection.column(1), projection.column(0))[-1]
        '''
        Here we check that the corners in HKL end up in the expected positions when transformed into the new scaled basis
        provided by the W transform (projection table)
        '''
        self.assertEquals(-(1/scale_x), out_md.getDimension(0).getMinimum()) 
        self.assertEquals((1/scale_x), out_md.getDimension(0).getMaximum())
        self.assertEquals(-(1/scale_y), out_md.getDimension(1).getMinimum())
        self.assertEquals((1/scale_y), out_md.getDimension(1).getMaximum())
        self.assertEquals((1/scale_z), out_md.getDimension(2).getMinimum())
        self.assertEquals(-(1/scale_z), out_md.getDimension(2).getMaximum())
        self.assertEquals("['2.00zeta', 0, 0]",  out_md.getDimension(0).getName() )
        self.assertEquals("[0, '2.00eta', 0]",  out_md.getDimension(1).getName() )
        self.assertEquals("[0, 0, '-4.00xi']",  out_md.getDimension(2).getName() )
                    

if __name__ == '__main__':
    unittest.main()