import unittest
import testhelpers
import numpy as np
from mantid.simpleapi import *
from mantid.api import IMDHistoWorkspace, IMDEventWorkspace


class CutMDTest(unittest.TestCase):
    

    def setUp(self):
        # Create a workspace
        data_ws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Mark the workspace as being in HKL
        SetSpecialCoordinates(InputWorkspace=data_ws, SpecialCoordinates='HKL')
        # Set the UB
        SetUB(Workspace=data_ws, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        # Add some data to the workspace
        FakeMDEventData(InputWorkspace=data_ws, PeakParams=[10000,0,0,0,1])
        self.__in_md  = data_ws
        
    def tearDown(self):
        DeleteWorkspace(self.__in_md )

    def test_exec_throws_if_not_a_hkl_workspace(self):
        test_md = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="A,B,C", Units="U,U,U")
        # Explicitly set the coordinate system to lab Q.
        SetSpecialCoordinates(InputWorkspace=test_md, SpecialCoordinates='Q (lab frame)')
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], CheckAxes=False)
        
    def test_exec_throws_if_set_to_be_a_hkl_workspace_but_with_missaligned_dimension_names(self):
        test_md = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="K,H,L", Units="U,U,U") # K,H,L are the dimension names
        SetSpecialCoordinates(InputWorkspace=test_md, SpecialCoordinates='HKL')
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], CheckAxes=True)
        
    def test_exec_throws_if_giving_4th_binning_parameter_when_workspace_is_3D(self):
        test_md = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names="H,K,L", Units="U,U,U")
        # Explicitly set the coordinate system to lab Q.
        SetSpecialCoordinates(InputWorkspace=test_md, SpecialCoordinates='HKL')
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=test_md, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], P4Bin=[0.1])
        
    def test_slice_to_original(self):
        out_md = CutMD(self.__in_md, P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], CheckAxes=False)
        self.assertTrue(isinstance(out_md, IMDEventWorkspace), "Should default to producing an IMDEventWorkspace.")
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
        self.assertTrue(isinstance(out_md, IMDEventWorkspace), "nopix defaults to True. Should get an IMDEventWorkspace")
        
    def test_recalculate_extents_with_3_bin_arguments(self):
        out_md = CutMD(self.__in_md, P1Bin=[0, 0.3, 0.8], P2Bin=[0.1], P3Bin=[0.1], CheckAxes=False, NoPix=True)
        dim = out_md.getDimension(0)
        self.assertAlmostEqual(0, dim.getMinimum(), 6, "Wrong minimum")
        self.assertEqual(2, dim.getNBins(), "Wrong calculated number of bins")
        self.assertAlmostEqual(0.6, dim.getMaximum(), 6, "Wrong calculated maximum")
        
    def test_truncate_extents(self):
        out_md = CutMD(self.__in_md, P1Bin=[0, 1.1, 1], P2Bin=[21], P3Bin=[0.1], CheckAxes=False, NoPix=True)
  
        self.assertEqual(1, out_md.getDimension(0).getNBins(), "Step is beyond range. Should just be integrated")
        self.assertEqual(1, out_md.getDimension(1).getNBins(), "Step is beyond range. Should just be integrated")
        
    def test_wrong_projection_workspace_format_wrong_column_numbers(self):
        projection = CreateEmptyTableWorkspace()
        projection.addColumn("double", "u")
        # missing other columns
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=self.__in_md, Projection=projection, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], CheckAxes=False)
        
    def test_wrong_table_workspace_format_wrong_row_numbers(self):
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("double", "w")
        projection.addColumn("double", "offset")
        projection.addColumn("string", "type")
        # Incorrect number of rows i.e. zero in this case as none added.
        self.assertRaises(RuntimeError, CutMD, InputWorkspace=self.__in_md, Projection=projection, OutputWorkspace="out_ws", P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], CheckAxes=False)
        
    def test_orthogonal_slice_with_scaling(self):
        # We create a fake workspace and check to see that the extents get scaled with the new coordinate system when sliced
        to_cut = CreateMDWorkspace(Dimensions=3, Extents=[-1,1,-1,1,-1,1], Names='H,K,L', Units='U,U,U')
        # Set the UB
        SetUB(Workspace=to_cut, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        
        SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')
        
        scale_x = 2.0
        scale_y = 2.0
                    
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("str", "type")
        projection.addRow([scale_x,0,"r"])
        projection.addRow([0,scale_y,"r"])  
        projection.addRow([0,0,"r"])   
                    
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
        
        
    def test_non_orthogonal_slice(self):
         # We create a fake workspace and check to see that the extents get transformed to the new coordinate system.
        to_cut = CreateMDWorkspace(Dimensions=3, Extents=[-1,1,-1,1,-1,1], Names='H,K,L', Units='U,U,U')
        # Set the UB
        SetUB(Workspace=to_cut, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')
        
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("double", "w")
        projection.addColumn("double", "offsets")
        projection.addColumn("str", "type")
        projection.addRow([1,-1, 0, 0, "r"])
        projection.addRow([1, 1, 0, 0, "r"])  
        projection.addRow([0, 0, 1, 0, "r"])  
                    
        out_md = CutMD(to_cut, Projection=projection, P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], NoPix=True)
        
        '''
        Here we check that the corners in HKL end up in the expected positions when transformed into the new scaled basis
        provided by the W transform (projection table)
        '''
        self.assertEquals(-1, out_md.getDimension(0).getMinimum()) 
        self.assertEquals(1, out_md.getDimension(0).getMaximum())
        self.assertEquals(-1, out_md.getDimension(1).getMinimum())
        self.assertEquals(1, out_md.getDimension(1).getMaximum())
        self.assertEquals(-1, out_md.getDimension(2).getMinimum())
        self.assertEquals(1, out_md.getDimension(2).getMaximum())
        self.assertEquals("['zeta', 'zeta', 0]",  out_md.getDimension(0).getName() )
        self.assertEquals("['-eta', 'eta', 0]",  out_md.getDimension(1).getName() )
        self.assertEquals("[0, 0, 'xi']",  out_md.getDimension(2).getName() )
        
        self.assertTrue(isinstance(out_md, IMDHistoWorkspace), "Expect that the output was an IMDHistoWorkspace given the NoPix flag.")
        
        run = out_md.getExperimentInfo(0).run()
        w_matrix = run.getLogData("W_MATRIX").value
        w_matrix = list(w_matrix)
        self.assertEquals([1,1,0,-1,1,0,0,0,1], w_matrix, "W-matrix should have been set, but should be an identity matrix")
        
    def test_orthogonal_slice_with_cropping(self):
         # We create a fake workspace and check to see that using bin inputs for cropping works
        to_cut = CreateMDWorkspace(Dimensions=3, Extents=[-1,1,-1,1,-1,1], Names='H,K,L', Units='U,U,U')
        # Set the UB
        SetUB(Workspace=to_cut, a = 1, b = 1, c = 1, alpha =90, beta=90, gamma = 90)
        SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')
        
        projection = CreateEmptyTableWorkspace()
        # Correct number of columns, and names
        projection.addColumn("double", "u")
        projection.addColumn("double", "v")
        projection.addColumn("double", "w")
        projection.addColumn("double", "offsets")
        projection.addColumn("str", "type")
        projection.addRow([1, 0, 0, 0, "r"])
        projection.addRow([0, 1, 0, 0, "r"])  
        projection.addRow([0, 0, 1, 0, "r"])  
                    
        '''
        Specify the cropping boundaries as part of the bin inputs.
        '''
        out_md = CutMD(to_cut, Projection=projection, P1Bin=[-0.5,0.5], P2Bin=[-0.1,0.1], P3Bin=[-0.3,0.3], NoPix=True)
        
        '''
        Here we check that the corners in HKL end up in the expected positions when transformed into the new scaled basis
        provided by the W transform (projection table)
        '''
        self.assertAlmostEqual(-0.5, out_md.getDimension(0).getMinimum(), 6) 
        self.assertAlmostEqual(0.5, out_md.getDimension(0).getMaximum(), 6) 
        self.assertAlmostEqual(-0.1, out_md.getDimension(1).getMinimum(), 6) 
        self.assertAlmostEqual(0.1, out_md.getDimension(1).getMaximum(), 6) 
        self.assertAlmostEqual(-0.3, out_md.getDimension(2).getMinimum(), 6) 
        self.assertAlmostEqual(0.3, out_md.getDimension(2).getMaximum(), 6) 
        self.assertEquals("['zeta', 0, 0]",  out_md.getDimension(0).getName() )
        self.assertEquals("[0, 'eta', 0]",  out_md.getDimension(1).getName() )
        self.assertEquals("[0, 0, 'xi']",  out_md.getDimension(2).getName() )
        
        self.assertTrue(isinstance(out_md, IMDHistoWorkspace), "Expect that the output was an IMDHistoWorkspace given the NoPix flag.")
        
        
                    

if __name__ == '__main__':
    unittest.main()