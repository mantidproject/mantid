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
        
    def test_endoverap_outside_range_throws(self):
        try:
            stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=self.x[0], EndOverlap=self.x[-1] + 0.001, Params='0.2')
            self.assertTrue(False, "Should have thrown with EndOverlap > x max")
        except RuntimeError:
            pass 
    
    def test_startoverap_outside_range_throws(self):
        try:
            stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=self.x[0]-0.001, EndOverlap=self.x[-1], Params='0.2')
            self.assertTrue(False, "Should have thrown with StartOverlap < x max")
        except RuntimeError:
            pass 
          
    def test_startoverap_greater_than_end_overlap_throws(self):
        try:
            stitched = Stitch1D(LHSWorkspace=self.b, RHSWorkspace=self.a, StartOverlap=self.x[-1], EndOverlap=self.x[0], Params='0.2')
            self.assertTrue(False, "Should have thrown with StartOverlap < x max")
        except RuntimeError:
            pass
          
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
        self.assertEquals(0, numpy.count_nonzero(eValues), "Output Error values should all be non-zero")
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
        self.assertEquals(0, numpy.count_nonzero(eValues), "Output Error values should all be non-zero")
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
        self.assertEquals(0, numpy.count_nonzero(eValues), "Output Error values should all be non-zero")
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
        self.assertEquals(0, numpy.count_nonzero(eValues), "Output Error values should all be non-zero")
        # Check that the output X-Values are correct.
        self.assertEquals(set(numpy.around(self.x, decimals=6)), set(xValues))     
        DeleteWorkspace(stitched[0]) 
 
if __name__ == '__main__':
    unittest.main()