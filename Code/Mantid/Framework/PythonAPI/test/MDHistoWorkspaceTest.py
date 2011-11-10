import unittest

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *

class MDHistoWorkspaceTest(unittest.TestCase):
    """
    Test the interface to MDHistoWorkspaces
    """
    
    def setUp(self):
        CreateMDWorkspace(Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',Units='m,m,m',SplitInto='5',MaxRecursionDepth='20',OutputWorkspace='mdw')
        FakeMDEventData("mdw",  UniformParams="1e4")
        BinToMDHistoWorkspace("mdw", "A",  1, "x,0,10,10", "y,0,10,10", "z,0,10,10", IterateEvents="1", Parallel="0")
        BinToMDHistoWorkspace("mdw", "B",  1, "x,0,10,10", "y,0,10,10", "z,0,10,10", IterateEvents="1", Parallel="0")
        pass
    
    def test_interface(self):
        A = mtd['A']
        self.assertEqual(A.getNumDims(), 3)  
        self.assertEqual(A.getNPoints(), 1000)
        # Can set/read signal and error
        A.setSignalAt(23, 123.0)  
        A.setErrorSquaredAt(23, 345.0)  
        self.assertEqual(A.signalAt(23), 123.0)  
        self.assertEqual(A.errorSquaredAt(23), 345.0)  
        
    """ Note: Look at each test for PlusMD MinusMD, and MDHistoWorkspaceTest for detailed tests including checking results.
    These tests only check that they do run. """
    def test_operators_md_md(self):
        A = mtd['A']
        B = mtd['B']
        C = A + B
        C = A * B
        C = A / B
        C = A - B
        A += B
        A *= B
        A /= B
        A -= B
        
    """ MDHistoWorkspace + a number """
    def test_operators_md_double(self):
        A = mtd['A']
        B = 3.5
        C = A + B
        C = A * B
        C = A / B
        C = A - B
        A += B
        A *= B
        A /= B
        A -= B
        

if __name__ == '__main__':
    unittest.main()

    
