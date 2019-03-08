from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.simpleapi import LoadEmptyInstrument, SANSMaskDTP, mtd


class SANSMaskDTPTest(unittest.TestCase):

    def test_cg2(self):
        ws_name = 'ws'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask just the first tube
        SANSMaskDTP(InputWorkspace=ws_name, tube="1")
        w = mtd[ws_name]
        detInfo = w.detectorInfo()
        start_id = 2  # First 2 are monitors
        for i in range(start_id, start_id+256):
            self.assertTrue(detInfo.isMasked(i),
                            "DetID={} is not masked".format(i))

    def test_cg2_top_bottom(self):
        ws_name = 'ws'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask the bottom and top of the detector
        SANSMaskDTP(InputWorkspace=ws_name, Pixel="1-10,247-256")
        w = mtd[ws_name]
        detInfo = w.detectorInfo()
        start_id = 2

        for tube in range(192):
            # top
            this_tube_first_id = start_id + 256*tube
            for i in range(this_tube_first_id, this_tube_first_id+10):
                self.assertTrue(detInfo.isMasked(i),
                                "DetID={} is not masked".format(i))

            # bottom
            this_tube_almost_last_id = start_id + 246 + 256*tube
            for i in range(this_tube_almost_last_id, this_tube_almost_last_id+10):
                self.assertTrue(detInfo.isMasked(i),
                                "DetID={} is not masked".format(i))

    def test_cg2_interleaved(self):
        ws_name = 'ws'
        LoadEmptyInstrument(InstrumentName='CG2', OutputWorkspace=ws_name)

        # Let's mask the bottom and top of the detector
        SANSMaskDTP(InputWorkspace=ws_name, Tube="1::2")
        w = mtd[ws_name]
        detInfo = w.detectorInfo()
        start_id = 2

        for tube in range(0, 192, 2):
            this_tube_first_id = start_id + 256*tube
            for i in range(this_tube_first_id, this_tube_first_id+256):
                self.assertTrue(detInfo.isMasked(i),
                                "DetID={} is not masked".format(i))


if __name__ == '__main__':
    unittest.main()
