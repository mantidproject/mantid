from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.kernel import Quat

class ComponentInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    def test_detectorsInSubtree(self):
        """ Test that a list of detectors is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.detectorsInSubtree(0)), list)

    def test_componentsInSubtree(self):
        """ Test that a list of components is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.componentsInSubtree(0)), list)

    def test_size(self):
        """ Check that there are only 6 components """
        info = self._ws.componentInfo()
        self.assertEquals(info.size(), 6)

    def test_isDetector(self):
        """Check which components are detectors """
        info = self._ws.componentInfo()
        self.assertEquals(info.isDetector(0), True)
        self.assertEquals(info.isDetector(1), True)
        self.assertEquals(info.isDetector(2), False)
        self.assertEquals(info.isDetector(3), False)
        self.assertEquals(info.isDetector(4), False)
        self.assertEquals(info.isDetector(5), False)

    def test_hasDetectorInfo(self):
        """ Check if detector information is available """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasDetectorInfo(), True)

    def test_position(self):
        """ Test that the component's position is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.position(0)), V3D)

    def test_rotation(self):
        """ Test that the component's rotation is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.rotation(0)), Quat)

    def test_relativePosition(self):
        """ Test that the component's relative position is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.relativePosition(0)), V3D)

    def test_relativeRotation(self):
        """ Test that the component's relative rotation is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.relativeRotation(0)), Quat)

    def test_setPosition(self):
        """ Test that the component's position can be set correctly. """
        info = self._ws.componentInfo()
        pos = V3D(0,0,0)
        info.setPosition(0, pos)
        retPos = info.position(0)
        self.assertEquals(pos, retPos)

    def test_setRotation(self):
        """ Test that the component's rotation can be set correctly. """
        info = self._ws.componentInfo()
        quat = Quat(0,0,0,0)
        info.setRotation(0, quat)
        retQuat = info.rotation(0)
        self.assertEquals(quat, retQuat)

    def test_sourcePosition(self):
        """ Check that the source postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.sourcePosition()), V3D)

    def test_samplePosition(self):
        """ Check that the sample postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.samplePosition()), V3D)

    def test_hasSource(self):
        """ Check that the source position is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasSource(), True)

    def test_hasSample(self):
        """ Check that the source postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasSample(), True)

    def test_name(self):
        """ Get the name of a component as a string """
        info = self._ws.componentInfo()

if __name__ == '__main__':
    unittest.main()
