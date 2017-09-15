from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper

class DetectorInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    def test_len(self):
        info = self._ws.detectorInfo()
        self.assertEquals(len(info), 2)

    def test_size(self):
        info = self._ws.detectorInfo()
        self.assertEquals(info.size(), 2)

    def test_isMasked(self):
        info = self._ws.detectorInfo()
        self.assertEquals(info.isMasked(0), False)
        self.assertEquals(info.isMasked(1), False)

if __name__ == '__main__':
    unittest.main()
