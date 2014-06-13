import unittest
import numpy
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

class Stitch1DTest(unittest.TestCase):
    
    a = None
    b = None
    x = None
    e = None
        
    def setUp(self):
        x = numpy.arange(-1, 1.2, 0.2)
        e = numpy.arange(-1, 1, 0.2)
        e.fill(0)
        self.e = e
        self.x = x
        a =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0,0,0,3,3,3,3,3,3,3], NSpec=1, DataE=e)
        b =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[2,2,2,2,2,2,2,0,0,0], NSpec=1, DataE=e)
        self.a = a
        self.b = b
    
    def tearDown(self):
        # Cleanup
        DeleteWorkspace(self.a)
        DeleteWorkspace(self.b)
        
    def test_startoverap_greater_than_end_overlap_throws(self):
        try:
            stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=self.x[-1], EndOverlap=self.x[0], Params='0.2')
            self.assertTrue(False, "Should have thrown with StartOverlap < x max")
        except RuntimeError:
            pass
  
    def test_lhsworkspace_must_be_histogram(self):
        x = numpy.arange(-1, 1, 0.2)
        e = numpy.arange(-1, 1, 0.2)
        lhs_ws =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0,0,0,3,3,3,3,3,3,3], NSpec=1, DataE=e)
        self.assertTrue(not lhs_ws.isHistogramData(), "Input LHS WS SHOULD NOT be histogram for this test")
        self.assertTrue(self.a.isHistogramData(), "Input RHS WS should SHOULD be histogram for this test")
        try:
            stitched = Stitch1D(LHSWorkspace=lhs_ws, RHSWorkspace=self.a, StartOverlap=self.x[-1], EndOverlap=self.x[0], Params='0.2')
        except ValueError:
            pass
        finally:
            DeleteWorkspace(lhs_ws)
    
    def test_rhsworkspace_must_be_histogram(self):
        x = numpy.arange(-1, 1, 0.2)
        e = numpy.arange(-1, 1, 0.2)
        rhs_ws =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0,0,0,3,3,3,3,3,3,3], NSpec=1, DataE=e)
        self.assertTrue(self.a.isHistogramData(), "Input LHS WS SHOULD be histogram for this test")
        self.assertTrue(not rhs_ws.isHistogramData(), "Input RHS WS should SHOULD NOT be histogram for this test")
        try:
            stitched = Stitch1D(LHSWorkspace=self.a, RHSWorkspace=rhs_ws, StartOverlap=self.x[-1], EndOverlap=self.x[0], Params='0.2')
        except ValueError:
            pass
        finally:
            DeleteWorkspace(rhs_ws)
            
    def test_stitching_uses_suppiled_params(self):
        stitched, scale = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=-0.4, EndOverlap=0.4, Params='-0.8, 0.2, 1')   
        
        #Check the ranges on the output workspace against the param inputs.
        
        out_x_values = stitched.readX(0)
        x_min = numpy.min(out_x_values)
        x_max = numpy.max(out_x_values)
        self.assertEqual(x_min, -0.8)
        self.assertEqual(x_max, 1)
        DeleteWorkspace(stitched)
                
    def test_stitching_determines_params(self):
        
        x1 = numpy.arange(-1, 1, 0.2)
        x2 = numpy.arange(0.4, 1.6, 0.2)
        ws1 =  CreateWorkspace(UnitX="1/q", DataX=x1, DataY=[1,1,1,1,1,1,1,1,1], NSpec=1)
        ws2 =  CreateWorkspace(UnitX="1/q", DataX=x2, DataY=[1,1,1,1,1,1], NSpec=1)
        
        demanded_step_size = 0.2
        stitched, scale = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2, StartOverlap=0.4, EndOverlap=1.0, Params=demanded_step_size)   
        
        #Check the ranges on the output workspace against the param inputs.
        
        out_x_values = stitched.readX(0)
        x_min = numpy.min(out_x_values)
        x_max = numpy.max(out_x_values)
        step_size = out_x_values[1] - out_x_values[0] 
        
        self.assertEqual(x_min, -1)
        self.assertAlmostEqual(x_max-demanded_step_size, 1.4, places=6)
        self.assertAlmostEqual(step_size, demanded_step_size, places=6)
        
        DeleteWorkspace(stitched)
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)
        

    def test_stitching_determines_start_and_end_overlap(self):
        x1 = numpy.arange(-1, 0.6, 0.2) # Produces x from -1 to 0.4 in steps of 0.2
        x2 = numpy.arange(-0.4, 1.2, 0.2) # Produces x from -0.4 to 1 in steps of 0.2
        ws1 =  CreateWorkspace(UnitX="1/q", DataX=x1, DataY=[1,1,1,3,3,3,3], NSpec=1)
        ws2 =  CreateWorkspace(UnitX="1/q", DataX=x2, DataY=[1,1,1,1,3,3,3], NSpec=1)
        
        stitched, scale = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2, Params=[-1, 0.2, 1]
                                   )  
        
        stitched_y = stitched.readY(0)
        stitched_x = stitched.readX(0)

        overlap_indexes = numpy.where((stitched_y >= 1.0009) & (stitched_y <= 3.0001))[0]
    
        start_overlap_determined = stitched_x[overlap_indexes[0]]
        end_overlap_determined = stitched_x[overlap_indexes[-1]]
        print start_overlap_determined, end_overlap_determined
        
        
        self.assertAlmostEqual(start_overlap_determined, -0.4, places=9)
        self.assertAlmostEqual(end_overlap_determined, 0.2, places=9) 
        
    def test_stitching_forces_start_overlap(self):
        x1 = numpy.arange(-1, 0.6, 0.2) # Produces x from -1 to 0.4 in steps of 0.2
        x2 = numpy.arange(-0.4, 1.2, 0.2) # Produces x from -0.4 to 1 in steps of 0.2
        ws1 =  CreateWorkspace(UnitX="1/q", DataX=x1, DataY=[1,1,1,3,3,3,3], NSpec=1)
        ws2 =  CreateWorkspace(UnitX="1/q", DataX=x2, DataY=[1,1,1,1,3,3,3], NSpec=1)
        
        # Overlap region is only physically between -0.4 and 0.4
        stitched, scale = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2, Params=[-1, 0.2, 1], StartOverlap=-0.5) # Start overlap is out of range!!  
        
        stitched_y = stitched.readY(0)
        stitched_x = stitched.readX(0)

        overlap_indexes = numpy.where((stitched_y >= 1.0009) & (stitched_y <= 3.0001))[0]
    
        start_overlap_determined = stitched_x[overlap_indexes[0]]
        end_overlap_determined = stitched_x[overlap_indexes[-1]]
        print start_overlap_determined, end_overlap_determined
        
        
        self.assertAlmostEqual(start_overlap_determined, -0.4, places=9)
        self.assertAlmostEqual(end_overlap_determined, 0.2, places=9) 
        
    def test_stitching_forces_end_overlap(self):
        x1 = numpy.arange(-1, 0.6, 0.2) # Produces x from -1 to 0.4 in steps of 0.2
        x2 = numpy.arange(-0.4, 1.2, 0.2) # Produces x from -0.4 to 1 in steps of 0.2
        ws1 =  CreateWorkspace(UnitX="1/q", DataX=x1, DataY=[1,1,1,3,3,3,3], NSpec=1)
        ws2 =  CreateWorkspace(UnitX="1/q", DataX=x2, DataY=[1,1,1,1,3,3,3], NSpec=1)
        
        # Overlap region is only physically between -0.4 and 0.4
        stitched, scale = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2, Params=[-1, 0.2, 1], EndOverlap=0.5) # End overlap is out of range!!  
        
        stitched_y = stitched.readY(0)
        stitched_x = stitched.readX(0)

        overlap_indexes = numpy.where((stitched_y >= 1.0009) & (stitched_y <= 3.0001))[0]
    
        start_overlap_determined = stitched_x[overlap_indexes[0]]
        end_overlap_determined = stitched_x[overlap_indexes[-1]]
        print start_overlap_determined, end_overlap_determined
        
        
        self.assertAlmostEqual(start_overlap_determined, -0.4, places=9)
        self.assertAlmostEqual(end_overlap_determined, 0.2, places=9) 
        
    def test_stitching_scale_right(self):
        stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=-0.4, EndOverlap=0.4, Params='0.2')    
        # Check the types returned 
        self.assertTrue(isinstance(stitched, tuple), "Output should be a tuple containing OuputWorkspace as well as the scale factor")
        self.assertTrue(isinstance(stitched[0], MatrixWorkspace))
        # Check the scale factor
        self.assertAlmostEquals(stitched[1], 2.0/3.0, places=9)
        # Fetch the arrays from the output workspace
        yValues = numpy.around(stitched[0].readY(0), decimals=6)
        eValues = numpy.around(stitched[0].readE(0), decimals=6)
        xValues = numpy.around(stitched[0].readX(0), decimals=6)
        # Check that the output Y-Values are correct.
        self.assertEquals(1, len(numpy.unique(yValues)), "Output YVaues should all be 2")
        self.assertEquals(2, yValues[0], "Output YValues should all be 2")
        # Check that the output E-Values are correct.
        self.assertEquals(0, len(eValues.nonzero()[0]), "Output Error values should all be non-zero")
        # Check that the output X-Values are correct.
        self.assertEquals(set(numpy.around(self.x, decimals=6)), set(xValues))
        DeleteWorkspace(stitched[0])
        

    def test_stitching_scale_left(self):
        stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=-0.4, EndOverlap=0.4, Params='0.2', ScaleRHSWorkspace=False)
    
        # Check the types returned 
        self.assertTrue(isinstance(stitched, tuple), "Output should be a tuple containing OuputWorkspace as well as the scale factor")
        self.assertTrue(isinstance(stitched[0], MatrixWorkspace))
        # Check the scale factor
        self.assertAlmostEquals(stitched[1], 3.0/2.0, places=9)
        # Fetch the arrays from the output workspace
        yValues = numpy.around(stitched[0].readY(0), decimals=6)
        eValues = numpy.around(stitched[0].readE(0), decimals=6)
        xValues = numpy.around(stitched[0].readX(0), decimals=6)
        # Check that the output Y-Values are correct.
        self.assertEquals(1, len(numpy.unique(yValues)), "Output YVaues should all be 2")
        self.assertEquals(3, yValues[0], "Output YValues should all be 3")
        # Check that the output E-Values are correct.
        self.assertEquals(0, len(eValues.nonzero()[0]), "Output Error values should all be non-zero")
        # Check that the output X-Values are correct.
        self.assertEquals(set(numpy.around(self.x, decimals=6)), set(xValues))     
        DeleteWorkspace(stitched[0]) 
        
    def test_stitching_manual_scale_factor_scale_right(self):
        stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, ScaleRHSWorkspace=True, UseManualScaleFactor=True,  StartOverlap=-0.4, EndOverlap=0.4, Params='0.2',  ManualScaleFactor=2.0/3.0)
        self.assertAlmostEquals(stitched[1], 2.0/3.0, places=9)
        # Fetch the arrays from the output workspace
        yValues = numpy.around(stitched[0].readY(0), decimals=6)
        eValues = numpy.around(stitched[0].readE(0), decimals=6)
        xValues = numpy.around(stitched[0].readX(0), decimals=6)
        # Check that the output Y-Values are correct.
        self.assertEquals(1, len(numpy.unique(yValues)), "Output YVaues should all be 2")
        self.assertEquals(2, yValues[0], "Output YValues should all be 2")
        # Check that the output E-Values are correct.
        self.assertEquals(0, len(eValues.nonzero()[0]), "Output Error values should all be non-zero")
        # Check that the output X-Values are correct.
        self.assertEquals(set(numpy.around(self.x, decimals=6)), set(xValues))     
        DeleteWorkspace(stitched[0]) 
    
    def test_stitching_manual_scale_factor_scale_left(self):
        stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=-0.4, EndOverlap=0.4, Params='0.2', ScaleRHSWorkspace=False, UseManualScaleFactor=True,  ManualScaleFactor=3.0/2.0)
        self.assertAlmostEquals(stitched[1], 3.0/2.0, places=9)
        # Fetch the arrays from the output workspace
        yValues = numpy.around(stitched[0].readY(0), decimals=6)
        eValues = numpy.around(stitched[0].readE(0), decimals=6)
        xValues = numpy.around(stitched[0].readX(0), decimals=6)
        # Check that the output Y-Values are correct.
        self.assertEquals(1, len(numpy.unique(yValues)), "Output YVaues should all be 2")
        self.assertEquals(3, yValues[0], "Output YValues should all be 3")
        # Check that the output E-Values are correct.
        self.assertEquals(0, len(eValues.nonzero()[0]), "Output Error values should all be non-zero")
        # Check that the output X-Values are correct.
        self.assertEquals(set(numpy.around(self.x, decimals=6)), set(xValues))     
        DeleteWorkspace(stitched[0]) 
   
if __name__ == '__main__':
    unittest.main()
