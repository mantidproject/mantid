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

    def test_geometry(self):
        info = self._ws.spectrumInfo()
        self.assertAlmostEquals(info.l2(1), 5.0009999)
        self.assertAlmostEquals(info.twoTheta(1), 0.01999733)
        self.assertAlmostEquals(info.phi(1), 1.57079632)
        p = info.position(1)
        self.assertEquals(p.X(), 0.0)
        self.assertEquals(p.Y(), 0.1)
        self.assertEquals(p.Z(), 5.0)

if __name__ == '__main__':
    unittest.main()
