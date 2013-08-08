import unittest
from mantid.simpleapi import *

class NormaliseToUnityTest(unittest.TestCase):
    """
        Simple test to check the numpy integration
    """
    
    def setUp(self):
        CreateWorkspace("normalise_to_unity_test", [1,2,3,4,5,6,1,2,3,4,5,6], [1,1,1,1,1,1,1,1,1,1], [1,1,1,1,1,1,1,1,1,1], 2)
        
    def test_whole_ws(self):
        """
            Check that we can normalize to the sum of all bins
        """
        output_ws = "output_1"
        NormaliseToUnity("normalise_to_unity_test", output_ws)
        self.assertEqual(mtd[output_ws].readY(0)[0],0.1)
        if mtd.doesExist(output_ws):
            DeleteWorkspace(output_ws)

    def test_x_range(self):
        """
            Check that we can specify a range in X and normalize to the sum in that range only
        """
        output_ws = "output_2"
        NormaliseToUnity("normalise_to_unity_test", output_ws, RangeLower=2, RangeUpper=4)
        self.assertEqual(mtd[output_ws].readY(0)[0],0.25)
        if mtd.doesExist(output_ws):
            DeleteWorkspace(output_ws)

    def test_x_range_and_spectra(self):
        """
            Check that we can specify both a range in X and a spectrum range
        """
        output_ws = "output_3"
        NormaliseToUnity("normalise_to_unity_test", output_ws, RangeLower=2, RangeUpper=4,
                         StartWorkspaceIndex=0, EndWorkspaceIndex=0)
        self.assertEqual(mtd[output_ws].readY(0)[0],0.5)
        if mtd.doesExist(output_ws):
            DeleteWorkspace(output_ws)
            
if __name__ == '__main__':
    unittest.main()
