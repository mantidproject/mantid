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
                self.assertTrue(not w.getInstrument().getDetector(int(i)).isMasked())
            else:
                self.assertTrue(w.getInstrument().getDetector(int(i)).isMasked())
        DeleteWorkspace(w)
        self.assertTrue(array_equal(masklist,arange(10)+10))

    def testFailNoInstrument(self):
        w1=CreateWorkspace(arange(5),arange(5))
        try:
            MaskAngle(w1,10,20)
            self.fail("Should not have got here. Should throw because no instrument.")
        except ValueError:
            pass
        finally:
            DeleteWorkspace(w1)

    def testFailLimits(self):
        w2=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('w2',w2)
        w3=CloneWorkspace('w2')
        w4=CloneWorkspace('w2')
        try:
            MaskAngle(w2,-100,20)
            self.fail("Should not have got here. Wrong angle.")
        except ValueError:
            pass
        finally:
            DeleteWorkspace('w2')
        try:
            MaskAngle(w3,10,200)
            self.fail("Should not have got here. Wrong angle.")
        except ValueError:
            pass
        finally:
            DeleteWorkspace('w3')
        try:
            MaskAngle(w4,100,20)
            self.fail("Should not have got here. Wrong angle.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace('w4')

if __name__ == '__main__':
    unittest.main()
