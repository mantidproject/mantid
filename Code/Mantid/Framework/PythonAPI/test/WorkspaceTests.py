import unittest

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *

class MatrixWorkspaceTest(unittest.TestCase):
    """
    Test the interface to MatrixWorkspaces
    """
    
    def setUp(self):
        pass
    
    def test_equals_matches_same_data(self):
       loq_1 = LoadRaw('LOQ48127.raw', 'loq_1', SpectrumMin=1, SpectrumMax=1).workspace()
       loq_2 = LoadRaw('LOQ48127.raw', 'loq_2', SpectrumMin=1, SpectrumMax=1).workspace()
       
       self.assertTrue(loq_1.equals(loq_2, 1e-08))

       DeleteWorkspace(loq_1)
       DeleteWorkspace(loq_2)

    def test_equals_doesnt_match_different_data(self):
       loq_1 = LoadRaw('LOQ48127.raw', 'loq_1', SpectrumMin=1, SpectrumMax=1).workspace()
       loq_2 = LoadRaw('LOQ48127.raw', 'loq_2', SpectrumMin=2, SpectrumMax=2).workspace()
       
       self.assertFalse(loq_1.equals(loq_2, 1e-08))

       DeleteWorkspace(loq_1)
       DeleteWorkspace(loq_2)

    def test_is_dirty(self):
        CreateWorkspace("test", DataX=1, DataY=1, DataE=1)
        self.assertFalse(mtd["test"].isDirty())
        Scale(InputWorkspace="test", OutputWorkspace="test", Factor=2.0)
        self.assertTrue(mtd["test"].isDirty())
        

if __name__ == '__main__':
    unittest.main()

    
