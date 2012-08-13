import unittest
from testhelpers import run_algorithm
from mantid import mtd

import numpy

class MDHistoWorkspaceTest(unittest.TestCase):
    """
    Test the interface to MDHistoWorkspaces
    """
    
    def setUp(self):
        run_algorithm('CreateMDWorkspace', Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',Units='m,m,m',SplitInto='5',
                      MaxRecursionDepth='20',OutputWorkspace='mdw')
        run_algorithm('FakeMDEventData', InputWorkspace="mdw",  UniformParams="1e4")
        run_algorithm('BinMD',InputWorkspace="mdw", OutputWorkspace="A", AxisAligned=True, AlignedDim0="x,0,10,10", AlignedDim1="y,0,10,10", 
                      AlignedDim2="z,0,10,10", IterateEvents="1", Parallel="0")
        run_algorithm('BinMD',InputWorkspace="mdw", OutputWorkspace="B", AxisAligned=True, AlignedDim0="x,0,10,10", AlignedDim1="y,0,10,10", 
                      AlignedDim2="z,0,10,10", IterateEvents="1", Parallel="0")
            
    def tearDown(self):
        for name in ('A','B','C','D','E','F','G','H'):
            mtd.remove(name)
        mtd.remove('mdw')
    
    def test_interface(self):
        A = mtd['A']
        self.assertEqual(A.getNumDims(), 3)  
        self.assertEqual(A.getNPoints(), 1000)
        # Can set/read signal and error
        A.setSignalAt(23, 123.0)  
        A.setErrorSquaredAt(23, 345.0)  
        self.assertEqual(A.signalAt(23), 123.0)  
        self.assertEqual(A.errorSquaredAt(23), 345.0)
        
    def test_signal_array_is_wrapped_in_read_only_numpy_array(self):
        run_algorithm('CreateMDHistoWorkspace', SignalInput='1,2,3,4,5,6,7,8,9',ErrorInput='1,1,1,1,1,1,1,1,1',
                      Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,3',Names='A,B',Units='U,T',OutputWorkspace='demo')
        testWS = mtd['demo']
        signal = testWS.getSignalArray()
        expected = numpy.array([[1,2,3],[4,5,6],[7,8,9]])
        self._verify_numpy_data(signal, expected)
        
        mtd.remove('demo')

    def test_errorSquared_array_is_wrapped_in_read_only_numpy_array(self):
        run_algorithm('CreateMDHistoWorkspace', SignalInput='1,2,3,4,5,6,7,8,9',ErrorInput='1,1,1,1,1,1,1,1,1',
                      Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,3',Names='A,B',Units='U,T',OutputWorkspace='demo')
        testWS = mtd['demo']
        errors = testWS.getErrorSquaredArray()
        expected = numpy.array([[1,1,1],[1,1,1],[1,1,1]])
        self._verify_numpy_data(errors, expected)
        
        mtd.remove('demo')
        
    def test_set_signal_array_throws_if_input_array_is_of_incorrect_size(self):
        run_algorithm('CreateMDHistoWorkspace', SignalInput='1,2,3,4,5,6,7,8,9',ErrorInput='1,1,1,1,1,1,1,1,1',
                      Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,3',Names='A,B',Units='U,T',OutputWorkspace='demo')
        testWS = mtd['demo']
        signal = numpy.array([1,2,3])
        self.assertRaises(ValueError, testWS.setSignalArray, signal)

    def test_set_signal_array_passes_numpy_values_to_workspace(self):
        run_algorithm('CreateMDHistoWorkspace', SignalInput='1,2,3,4,5,6,7,8,9',ErrorInput='1,1,1,1,1,1,1,1,1',
                      Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,3',Names='A,B',Units='U,T',OutputWorkspace='demo')
        testWS = mtd['demo']
        signal = numpy.arange(10,19,dtype=numpy.float64)
        signal = numpy.reshape(signal,(3,3))
        testWS.setSignalArray(signal)
        new_signal = testWS.getSignalArray()
        self._verify_numpy_data(new_signal, signal)


    def test_set_error_array_passes_numpy_values_to_workspace(self):
        run_algorithm('CreateMDHistoWorkspace', SignalInput='1,2,3,4,5,6,7,8,9',ErrorInput='1,1,1,1,1,1,1,1,1',
                      Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,3',Names='A,B',Units='U,T',OutputWorkspace='demo')
        testWS = mtd['demo']
        errors = numpy.arange(20,29,dtype=numpy.float64)
        errors = numpy.reshape(errors,(3,3))
        testWS.setErrorSquaredArray(errors)
        new_errors = testWS.getErrorSquaredArray()
        self._verify_numpy_data(new_errors, errors)

    def _verify_numpy_data(self, test_array, expected):
        """Check the correct numpy array has been constructed"""

        self.assertTrue(isinstance(test_array, numpy.ndarray))
        self.assertTrue(len(expected.shape), len(test_array.shape))
        self.assertTrue(not numpy.all(test_array.flags.writeable))
        self.assertTrue(numpy.all(numpy.equal(expected, test_array)))
        
        
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
        self.assertTrue(C is not None)

    """ boolean_workspace = MDHistoWorkspace < MDHistoWorkspace """
    def test_comparisons_and_boolean_operations(self):
        A = mtd['A']
        B = mtd['B']
        B += 1
        C = A < B
        self.assertEqual( C.name(), 'C')
        self.assertEqual( C.signalAt(0), 1.0)
        D = A > B
        self.assertEqual( D.name(), 'D')
        self.assertEqual( D.signalAt(0), 0.0)
        E = C | D
        self.assertEqual( E.name(), 'E')
        self.assertEqual( E.signalAt(0), 1.0)
        F = C & D
        self.assertEqual( F.name(), 'F')
        self.assertEqual( F.signalAt(0), 0.0)
        G = C ^ C
        self.assertEqual( G.name(), 'G')
        self.assertEqual( G.signalAt(0), 0.0)
        H = C ^ D
        self.assertEqual( H.name(), 'H')
        self.assertEqual( H.signalAt(0), 1.0)
        
    def test_comparisons_histo_scalar(self):
        A = mtd['A']
        C = A < 1000.0
        self.assertEqual( C.name(), 'C')
        self.assertEqual( C.signalAt(0), 1.0)
        D = A > 1.0
        self.assertEqual( D.name(), 'D')
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
        self.assertTrue(C is not None)

        
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

    
