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
        a =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0.0,0.0,0.0,3.0,3.0,3.0,3.0,3.0,3.0,3.0], NSpec=1, DataE=e)
        b =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[2.0,2.0,2.0,2.0,2.0,2.0,2.0,0.0,0.0,0.0], NSpec=1, DataE=e)
        c =  CreateWorkspace(UnitX="1/q", DataX=x, DataY=[2.0,2.0,2.0,2.0,2.0,2.0,2.0,0.0,0.0,0.0], NSpec=1, DataE=e)
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
            stitched = Stitch1DMany(InputWorkspaces='a', StartOverlaps=[-0.5], EndOverlaps=[0.5], Params=[0.1])
            self.fail("Only one workspace. Should have thrown.")
        except RuntimeError:
            pass
       
    def test_stitch_throws_with_wrong_number_of_Start_overlaps(self):
        try:
            stitched = Stitch1DMany(InputWorkspaces='a, b', StartOverlaps=[-0.5, -0.6], EndOverlaps=[0.5], Params=[0.1])
            self.fail("Two start overlaps, but only two workspaces. Should have thrown.")
        except RuntimeError:
            pass
            
    def test_stitch_throws_with_wrong_number_of_End_overlaps(self):
        try:
            stitched = Stitch1DMany(InputWorkspaces='a, b', StartOverlaps=[-0.5], EndOverlaps=[0.5, 0.6], Params=[0.1])
            self.fail("Two end overlaps, but only two workspaces. Should have thrown.")
        except RuntimeError:
            pass
         
    '''
    Cross-check that the result of using Stitch1DMany with two workspaces is the same as using Stitch1D.    
    '''
    def test_stitches_two(self):
        stitchedViaStitchMany, scaleFactorMany = Stitch1DMany(InputWorkspaces='a, b', StartOverlaps=[-0.4], EndOverlaps=[0.4], Params=[0.2])
        stitchedViaStitchTwo, scaleFactorTwo = Stitch1D(LHSWorkspace=self.a, RHSWorkspace=self.b, StartOverlap=-0.4, EndOverlap=0.4, Params=[0.2])
        self.assertEquals(scaleFactorTwo, scaleFactorMany)
        
        yDataRounded = [ round(elem, 4) for elem in stitchedViaStitchMany.readY(0) ]
        expectedYData = [0,0,0,3,3,3,3,0,0,0]
        same = all([(x == y)  for x,y in zip(yDataRounded, expectedYData)])
        self.assertTrue(same)
        
        # Do cross compare
        isSuccess = CheckWorkspacesMatch(Workspace1=stitchedViaStitchMany, Workspace2=stitchedViaStitchTwo)
        self.assertEquals("Success!", isSuccess);
        
if __name__ == '__main__':
    unittest.main()