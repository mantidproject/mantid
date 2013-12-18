import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *

class MaskAngleTest(unittest.TestCase):
          
    def testMaskAngle(self):
        w=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('w',w)
        masklist = MaskAngle(w,10,20)
        for i in arange(w.getNumberHistograms())+1:
            if (i<10) or (i>19):
                self.assertTrue(not w.getInstrument().getDetector(i).isMasked())
            else:
                self.assertTrue(w.getInstrument().getDetector(i).isMasked())
        DeleteWorkspace(w)
        self.assertTrue(array_equal(masklist,arange(10)+10))
    
 
if __name__ == '__main__':
    unittest.main()
