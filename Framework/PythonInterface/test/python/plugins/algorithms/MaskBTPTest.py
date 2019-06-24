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

# tests run x10 slower with this on, but it may be useful to track down issues refactoring
CHECK_CONSISTENCY = False


class MaskBTPTest(unittest.TestCase):
    def checkConsistentMask(self, wksp, masked):
        if not CHECK_CONSISTENCY:
            return
        compInfo = wksp.componentInfo()
        detInfo = wksp.detectorInfo()
        # detector ids are any number, detector index are 0->number of detectors
        detIds = detInfo.detectorIDs()
        for detIndex, detId in enumerate(detIds):
            if not compInfo.isDetector(detIndex):
                continue

            if detInfo.isMonitor(detIndex):
                self.assertFalse(detInfo.isMasked(detIndex),
                                 'DetID={} is a monitor and shouldn\'t be masked'.format(detId))
            else:
                self.assertEqual(detInfo.isMasked(detIndex), detId in masked,
                                 'DetID={} is has incorrect mask bit. "{}" should be "{}"'.format(detId,
                                                                                                  detInfo.isMasked(int(detId)),
                                                                                                  detId in masked))

    def checkDetectorIndexes(self, wksp, detIndices):
        '''This is use to spot check specific detector indices (not identifiers) as masked'''
        detInfo = wksp.detectorInfo()
        for detIndex in detIndices:
            self.assertTrue(detInfo.isMasked(detIndex),
                            'Detector index={} should be masked'.format(detIndex))

    def testMaskBTPWrongInstrument(self):
        w=WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30,5,False,False)
        AnalysisDataService.add('w',w)
        try:
            MaskBTP(Workspace=w,Pixel="1")
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
        detIds = detInfo.detectorIDs()

        # check for some to be masked
        for id in (29696, 29697, 29698, #29699,
                   29700, 1020, 4400):
            index = int(where(detIds == id)[0][0])
            self.assertTrue(detInfo.isMasked(index),
                            msg='detId={}, index={} should not be masked'.format(id, index))

        # check for some to not be masked
        for id in [3071]:
            index = int(where(detIds == id)[0][0])
            self.assertFalse(detInfo.isMasked(index),
                             msg='detId={}, index={} should not be masked'.format(id, index))
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
        self.assertEqual(4 * 25, len(masking))

        # keep on masking the same workspace to speed up the test
        masking = MaskBTP(Workspace='TOPAZMaskBTP', Tube='edges')
        self.assertEqual(2 * 256 * 25, len(masking))

    def test_cg2(self):
        ws_name = 'cg2'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask just the first tube
        masked = MaskBTP(Workspace=ws_name, tube="1")
        wksp = mtd[ws_name]
        self.assertEqual(int(256), len(masked))
        self.checkConsistentMask(wksp, masked)

        # check for specific mask values
        start_index = 2  # First 2 are monitors
        self.checkDetectorIndexes(wksp, list(range(start_index, start_index+256)))

    def test_cg2_top_bottom(self):
        ws_name = 'cg2_top_bottom'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask the bottom and top of the detector
        masked = MaskBTP(Workspace=ws_name, Pixel="1-10,247-256")
        wksp = mtd[ws_name]
        self.assertEqual(int(192*20), len(masked))
        self.checkConsistentMask(wksp, masked)

        # check for specific mask values
        start_id = 2
        for tube in range(192):
            # top
            this_tube_first_id = start_id + 256 * tube
            self.checkDetectorIndexes(wksp, list(range(this_tube_first_id, this_tube_first_id + 10)))

            # bottom
            this_tube_almost_last_id = start_id + 246 + 256 * tube
            self.checkDetectorIndexes(wksp, list(range(this_tube_almost_last_id, this_tube_almost_last_id + 10)))

    def test_cg2_interleaved(self):
        ws_name = 'cg2_interleaved'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask the bottom and top of the detector
        masked = MaskBTP(Workspace=ws_name, Tube="1:300:2")
        wksp = mtd[ws_name]
        self.assertEqual(int(192*256/2), len(masked))
        self.checkConsistentMask(wksp, masked)

        # check for specific mask values
        start_id = 2
        for tube in range(0, 192, 2):
            this_tube_first_id = start_id + 256*tube
            self.checkDetectorIndexes(wksp, list(range(this_tube_first_id, this_tube_first_id+256)))

    def test_eqsans_interleaved(self):
        ws_name = 'eqsans'
        LoadEmptyInstrument(InstrumentName='EQSANS', OutputWorkspace=ws_name)

        masked = MaskBTP(Workspace=ws_name, Tube="5:200:8,6:200:8,7:200:8,8:200:8")
        wksp = mtd[ws_name]
        self.assertEqual(int(192*256/2), len(masked))
        self.checkConsistentMask(wksp, masked)

        # check for specific mask values
        masked = [i + 1 for i in range(256*4, 256 * 8 * 24, 2048)]  # overwrite previous version
        self.checkDetectorIndexes(wksp, masked)

    def test_biosans_wing_plane(self):
        ws_name = 'biosans_wing'
        LoadEmptyInstrument(InstrumentName='BIOSANS', OutputWorkspace=ws_name)

        masked = MaskBTP(Workspace=ws_name, Bank=2, Tube='1:300:2')
        wksp = mtd[ws_name]
        self.assertEqual(int(160 * 256 / 2), len(masked))
        self.checkConsistentMask(wksp, masked)

    def test_biosans_wing_ends(self):
        masked = MaskBTP(Instrument='BIOSANS', Bank=2, Pixel='1-20,245-256')
        wksp = mtd['BIOSANSMaskBTP']
        self.assertEqual(int(32 * 160), len(masked))
        self.checkConsistentMask(wksp, masked)


if __name__ == '__main__':
    unittest.main()
