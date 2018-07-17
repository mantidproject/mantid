from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.kernel import Quat
from mantid.simpleapi import *

class DetectorInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        """ Setup Workspace to use"""
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    def test_len(self):
        """ Test return value for number of detectors """
        info = self._ws.detectorInfo()
        self.assertEquals(len(info), 2)

    def test_size(self):
        """ Test return value for number of detectors """
        info = self._ws.detectorInfo()
        self.assertEquals(info.size(), 2)

    def test_isMasked(self):
        """ Check masking of detector """
        info = self._ws.detectorInfo()
        self.assertEquals(info.isMasked(0), False)
        self.assertEquals(info.isMasked(1), False)

    def test_isEquivalent(self):
        """ Check equality of detectors """
        info = self._ws.detectorInfo()
        self.assertTrue(info.isEquivalent(info))
        self.assertTrue(info == info)

    def test_twoTheta(self):
        """ See if the returned value is a double (float in Python). """
        info = self._ws.detectorInfo()
        self.assertEquals(type(info.twoTheta(0)), float)

    def test_setMasked(self):
        """ Test that the detector's masking can be set to True. """
        info = self._ws.detectorInfo()
        info.setMasked(0, True)
        self.assertTrue(info.isMasked(0))

    def test_clearMaskFlags(self):
        """ Test that the detector's masking can be cleared. """
        info = self._ws.detectorInfo()
        info.setMasked(0, True)
        self.assertTrue(info.isMasked(0))
        info.clearMaskFlags()
        self.assertFalse(info.isMasked(0))

    """
    The following test cases test for returned objects to do with position
    and rotation.
    """

    def test_position(self):
        """ Test that the detector's position is returned. """
        info = self._ws.detectorInfo()
        self.assertEquals(type(info.position(0)), V3D)

    def test_rotation(self):
        """ Test that the detector's rotation is returned. """
        info = self._ws.detectorInfo()
        self.assertEquals(type(info.rotation(0)), Quat)

    def test_createWorkspaceAndDetectorInfo(self):
    	""" Try to create a workspace and see if DetectorInfo object
    		is accessable """
    	dataX = [1,2,3,4,5]
    	dataY = [1,2,3,4,5]
    	workspace = CreateWorkspace(DataX=dataX, DataY=dataY)
    	info = workspace.detectorInfo()
    	self.assertEquals(info.size(), 0)


if __name__ == '__main__':
    unittest.main()
