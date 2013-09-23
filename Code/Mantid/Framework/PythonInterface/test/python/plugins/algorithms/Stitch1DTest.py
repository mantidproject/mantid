import unittest
import numpy
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

class Stitch1DTest(unittest.TestCase):
          
    def test_stitching_scale_right(self):
        x = numpy.arange(-1, 1.2, 0.2)
        e = numpy.arange(-1, 1, 0.2)
        e.fill(0)
        a =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0,0,0,3,3,3,3,3,3,3], NSpec=1, DataE=e)
        b =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[2,2,2,2,2,2,2,0,0,0], NSpec=1, DataE=e)
        stitched = Stitch1D(LHSWorkspace=b, RHSWorkspace=a, StartOverlap=-0.4, EndOverlap=0.4, Params='0.2')
    
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
        self.assertEquals(set(numpy.around(x, decimals=6)), set(xValues))
        # Cleanup
        DeleteWorkspace(a)
        DeleteWorkspace(b)
        DeleteWorkspace(stitched[0])
        
    def test_stitching_scale_left(self):
        x = numpy.arange(-1, 1.2, 0.2)
        e = numpy.arange(-1, 1, 0.2)
        e.fill(0)
        a =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0,0,0,3,3,3,3,3,3,3], NSpec=1, DataE=e)
        b =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[2,2,2,2,2,2,2,0,0,0], NSpec=1, DataE=e)
        stitched = Stitch1D(LHSWorkspace=b, RHSWorkspace=a, StartOverlap=-0.4, EndOverlap=0.4, Params='0.2', ScaleRHSWorkspace=False)
    
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
        self.assertEquals(set(numpy.around(x, decimals=6)), set(xValues))
        # Cleanup
        DeleteWorkspace(a)
        DeleteWorkspace(b)
        DeleteWorkspace(stitched[0])

    
        
 
if __name__ == '__main__':
    unittest.main()