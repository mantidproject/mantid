from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.simpleapi import LoadEmptyInstrument, SANSMaskDTP, mtd

# tests run x10 slower with this on, but it may be useful to track down issues refactoring
CHECK_CONSISTENCY = False


class SANSMaskDTPTest(unittest.TestCase):
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


    def test_cg2(self):
        ws_name = 'cg2'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask just the first tube
        masked = SANSMaskDTP(InputWorkspace=ws_name, tube="1")
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
        masked = SANSMaskDTP(InputWorkspace=ws_name, Pixel="1-10,247-256")
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
        masked = SANSMaskDTP(InputWorkspace=ws_name, Tube="1:300:2")
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

        masked = SANSMaskDTP(InputWorkspace=ws_name, Tube="5:200:8,6:200:8,7:200:8,8:200:8")
        wksp = mtd[ws_name]
        self.assertEqual(int(192*256/2), len(masked))
        self.checkConsistentMask(wksp, masked)

        # check for specific mask values
        masked = [i + 1 for i in range(256*4, 256 * 8 * 24, 2048)]  # overwrite previous version
        self.checkDetectorIndexes(wksp, masked)

    def test_biosans_wing_plane(self):
        ws_name = 'biosans_wing'
        LoadEmptyInstrument(InstrumentName='BIOSANS', OutputWorkspace=ws_name)

        masked = SANSMaskDTP(InputWorkspace=ws_name, Detector=2, Tube='1:300:2')
        wksp = mtd[ws_name]
        self.assertEqual(int(160 * 256 / 2), len(masked))
        self.checkConsistentMask(wksp, masked)

    def test_biosans_wing_ends(self):
        ws_name = 'biosans_wing'
        LoadEmptyInstrument(InstrumentName='BIOSANS', OutputWorkspace=ws_name)

        masked = SANSMaskDTP(InputWorkspace=ws_name, Detector=2, Pixel='1-20,245-256')
        wksp = mtd[ws_name]
        self.assertEqual(int(32 * 160), len(masked))
        self.checkConsistentMask(wksp, masked)

if __name__ == '__main__':
    unittest.main()
