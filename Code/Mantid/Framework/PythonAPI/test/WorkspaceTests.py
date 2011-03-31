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
       alf_1 = LoadRaw('ALF15739.raw', 'alf_1', SpectrumMin=1, SpectrumMax=1).workspace()
       alf_2 = LoadRaw('ALF15739.raw', 'alf_2', SpectrumMin=1, SpectrumMax=1).workspace()
       
       self.assertTrue(alf_1.equals(alf_2, 1e-08))

       DeleteWorkspace(alf_1)
       DeleteWorkspace(alf_2)

    def test_equals_doesnt_match_different_data(self):
       alf_1 = LoadRaw('ALF15739.raw', 'alf_1', SpectrumMin=1, SpectrumMax=1).workspace()
       alf_2 = LoadRaw('ALF15739.raw', 'alf_2', SpectrumMin=2, SpectrumMax=2).workspace()
       
       self.assertFalse(alf_1.equals(alf_2, 1e-08))

       DeleteWorkspace(alf_1)
       DeleteWorkspace(alf_2)

    def test_is_dirty(self):
        CreateWorkspace("test", DataX=1, DataY=1, DataE=1)
        self.assertFalse(mtd["test"].isDirty())
        Scale(InputWorkspace="test", OutputWorkspace="test", Factor=2.0)
        self.assertTrue(mtd["test"].isDirty())
        

if __name__ == '__main__':
    unittest.main()

    
