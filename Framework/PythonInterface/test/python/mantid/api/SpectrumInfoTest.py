from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper

class SpectrumInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    def test_hasDetectors(self):
        info = self._ws.spectrumInfo()
        self.assertEquals(info.hasDetectors(0), False)
        self.assertEquals(info.hasDetectors(1), True)

    def test_isMasked(self):
        info = self._ws.spectrumInfo()
        self.assertEquals(info.isMasked(1), False)

if __name__ == '__main__':
    unittest.main()
