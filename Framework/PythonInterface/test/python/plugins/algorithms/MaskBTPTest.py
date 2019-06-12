# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

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
            MaskBTP(Instrument='SEQUOIA', Bank="22")
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
        detInfo = w.detectorInfo()
        self.assertTrue(detInfo.isMasked(29699)) #pixel1 (detID 29696)
        self.assertTrue(detInfo.isMasked(29700)) #pixel2 (detID 29697)
        self.assertTrue(detInfo.isMasked(29701)) #pixel3 (detID 29698)
        self.assertFalse(detInfo.isMasked(29702)) #pixel4 (detID 29699)
        self.assertTrue(detInfo.isMasked(29703)) #pixel5 (detID 29700)

        self.assertTrue(detInfo.isMasked(1023)) #bank1 (detID 1020)
        self.assertFalse(detInfo.isMasked(3071)) #bank3, tube 8 (detID 3068)

        self.assertTrue(detInfo.isMasked(4403)) #bank5, tube 3 (detID 4400)
        DeleteWorkspace(w)

    def testSEQMaskBTP(self):
        MaskBTP(Instrument='SEQUOIA', Bank="23")
        MaskBTP(Instrument='SEQUOIA', Bank="24")
        MaskBTP(Instrument='SEQUOIA', Bank="25")
        MaskBTP(Instrument='SEQUOIA', Bank="26")
        MaskBTP(Instrument='SEQUOIA', Bank="27")
        MaskBTP(Instrument='SEQUOIA', Bank="37")
        MaskBTP(Instrument='SEQUOIA', Bank="38")
        return

    def testEdges(self):
        # this combined option should probably be called corners
        masking = MaskBTP(Instrument='TOPAZ', Tube='edges', Pixel='EdGes')  # funny case just b/c
        self.assertEqual(4 * 20, len(masking))

        # keep on masking the same workspace to speed up the test
        masking = MaskBTP(Workspace='TOPAZMaskBTP', Tube='edges')
        self.assertEqual(2 * 256 * 20, len(masking))


if __name__ == '__main__':
    unittest.main()
