import unittest
from mantid.simpleapi import *
from mantid.api import *
from testhelpers import *
from numpy import *

class MaskBTPTest(unittest.TestCase):

    def testMaskBTPWrongInstrument(self):
        w=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('w',w)
        try:
            masklist = MaskBTP(Workspace=w,Pixel="1")
            self.fail("Should not have got here. Should throw because wrong instrument.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(w)

    def testMaskBTPWrongLimits(self):
        try:
            MaskBTP(Instrument='ARCS', Pixel="129")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        try:
            MaskBTP(Instrument='SEQUOIA', Bank="1")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        try:
            MaskBTP(Instrument='HYSPEC', Tube="18")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        DeleteWorkspace("ARCSMaskBTP")
        DeleteWorkspace("HYSPECMaskBTP")
        DeleteWorkspace("SEQUOIAMaskBTP")

    def testMaskBTP(self):
        m1=MaskBTP(Instrument='CNCS', Pixel="1-3,5")
        m2=MaskBTP(Workspace='CNCSMaskBTP', Bank="1-2")
        m3=MaskBTP(Workspace='CNCSMaskBTP', Bank='5-7', Tube='3')
        p1=arange(400)*128
        m1p=sort(concatenate((p1,p1+1,p1+2,p1+4)))
        self.assertTrue(array_equal(m1,m1p))
        self.assertTrue(array_equal(m2,arange(2048)))
        b5t3=arange(128)+4*1024+2*128
        self.assertTrue(array_equal(m3,concatenate((b5t3,b5t3+1024,b5t3+2048))))
        #check whether some pixels are masked when they should
        w=mtd['CNCSMaskBTP']
        self.assertTrue(w.getInstrument().getDetector(29696).isMasked()) #pixel1
        self.assertTrue(w.getInstrument().getDetector(29697).isMasked()) #pixel2
        self.assertTrue(w.getInstrument().getDetector(29698).isMasked()) #pixel3
        self.assertTrue(not w.getInstrument().getDetector(29699).isMasked()) #pixel4
        self.assertTrue(w.getInstrument().getDetector(29700).isMasked()) #pixel5

        self.assertTrue(w.getInstrument().getDetector(1020).isMasked()) #bank 1
        self.assertTrue(not w.getInstrument().getDetector(3068).isMasked()) #bank3, tube 8

        self.assertTrue(w.getInstrument().getDetector(4400).isMasked()) #bank5, tube 3
        DeleteWorkspace(w)

if __name__ == '__main__':
    unittest.main()
