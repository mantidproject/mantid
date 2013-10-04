import unittest
import numpy
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

class Stitch1DManyTest(unittest.TestCase):
    
    a = None
    b = None
    c = None
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
        c =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[2,2,2,2,2,2,2,0,0,0], NSpec=1, DataE=e)
        self.a = a
        self.b = b
        self.c = c
        
        
    
    def tearDown(self):
        # Cleanup
        DeleteWorkspace(self.a)
        DeleteWorkspace(self.b)
        DeleteWorkspace(self.c)
        
    def test_stitch_throws_with_too_few_workspaces(self):
        try:
            stitched = Stitch1DMany(InputWorkspaces='a', StartOverlaps=[-0.5], EndOverlaps=[0.5], Params='0.1')
            self.fail("Only one workspace. Should have thrown.")
        except ValueError:
            pass
        
if __name__ == '__main__':
    unittest.main()