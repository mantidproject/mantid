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
        BinMD(InputWorkspace="mdw", OutputWorkspace="A",  AxisAligned='1', AlignedDim0="x,0,10,10", AlignedDim1="y,0,10,10", AlignedDim2="z,0,10,10", IterateEvents="1", Parallel="0")
        BinMD(InputWorkspace="mdw", OutputWorkspace="B",  AxisAligned='1', AlignedDim0="x,0,10,10", AlignedDim1="y,0,10,10", AlignedDim2="z,0,10,10", IterateEvents="1", Parallel="0")
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
        
    def test_compound_arithmetic(self):
        A = mtd['A']
        B = mtd['B']
        C = (A + B) / (A - B) 
        assert (C is not None)

    """ boolean_workspace = MDHistoWorkspace < MDHistoWorkspace """
    def test_comparisons_and_boolean_operations(self):
        A = mtd['A']
        B = mtd['B']
        B += 1
        C = A < B
        self.assertEqual( C.getName(), 'C')
        self.assertEqual( C.signalAt(0), 1.0)
        D = A > B
        self.assertEqual( D.getName(), 'D')
        self.assertEqual( D.signalAt(0), 0.0)
        E = C | D
        self.assertEqual( E.getName(), 'E')
        self.assertEqual( E.signalAt(0), 1.0)
        F = C & D
        self.assertEqual( F.getName(), 'F')
        self.assertEqual( F.signalAt(0), 0.0)
        G = C ^ C
        self.assertEqual( G.getName(), 'G')
        self.assertEqual( G.signalAt(0), 0.0)
        H = C ^ D
        self.assertEqual( H.getName(), 'H')
        self.assertEqual( H.signalAt(0), 1.0)
        
    def test_comparisons_histo_scalar(self):
        A = mtd['A']
        C = A < 1000.0
        self.assertEqual( C.getName(), 'C')
        self.assertEqual( C.signalAt(0), 1.0)
        D = A > 1.0
        self.assertEqual( D.getName(), 'D')
        self.assertEqual( D.signalAt(0), 1.0)

    def test_inplace_boolean_operations(self):
        A = mtd['A']
        B = mtd['B']
        B += 1
        C = A < B # all 1 (true)
        D = A > B # all 0 (false)
        
        C |= D
        self.assertEqual( C.signalAt(0), 1.0)
        C &= D
        self.assertEqual( C.signalAt(0), 0.0)
        C += 1
        self.assertEqual( C.signalAt(0), 1.0)
        C ^= C
        self.assertEqual( C.signalAt(0), 0.0)
        
    def test_not_operator(self):
        A = mtd['A']
        A *= 0
        self.assertEqual( A.signalAt(0), 0.0)
        # Do with a copy
        B = ~A
        self.assertEqual( B.signalAt(0), 1.0)
        # Do in-place
        A = ~A
        self.assertEqual( A.signalAt(0), 1.0)

    def test_compound_comparison(self):
        A = mtd['A']
        B = mtd['B']
        C = (A > B) & (A > 123) & (B < 2345)
        assert (C is not None)

        
    def test_compound_boolean_operations(self):
        A = mtd['A']
        A *= 0
        B = A + 1
        C = ~(A | B) 
        self.assertEqual( C.signalAt(0), 0.0)
        C = ~(A | B) | B 
        self.assertEqual( C.signalAt(0), 1.0)
        C = ~(A | B) | ~A 
        self.assertEqual( C.signalAt(0), 1.0)
        C = ~(A | B) | ~(A & B) 
        self.assertEqual( C.signalAt(0), 1.0)
        
if __name__ == '__main__':
    unittest.main()

    
