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

    """
    ----------------------------------------------------------------------------
    Normal Tests
    ----------------------------------------------------------------------------

    The following test cases test normal usage of the exposed methods.
    """

    def test_len(self):
        """ Test return value for number of detectors """
        info = self._ws.detectorInfo()
        self.assertEquals(len(info), 2)

    def test_size(self):
        """ Test return value for number of detectors """
        info = self._ws.detectorInfo()
        self.assertEquals(info.size(), 2)

    def test_isMonitor(self):
        """ Check if detector is a monitor """
        info = self._ws.detectorInfo()
        self.assertEquals(info.isMonitor(0), False)
        self.assertEquals(info.isMonitor(1), False)

    def test_isMasked(self):
        """ Check masking of detector """
        info = self._ws.detectorInfo()
        self.assertEquals(info.isMasked(0), False)
        self.assertEquals(info.isMasked(1), False)

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

    def test_isEquivalent(self):
        """ Check equality of detectors """
        info = self._ws.detectorInfo()
        self.assertTrue(info.isEquivalent(info))
        self.assertTrue(info == info)

    def test_twoTheta(self):
        """ See if the returned value is a double (float in Python). """
        info = self._ws.detectorInfo()
        self.assertEquals(type(info.twoTheta(0)), float)

    def test_createWorkspaceAndDetectorInfo(self):
    	""" Try to create a workspace and see if DetectorInfo object
    		is accessable """
    	dataX = [1,2,3,4,5]
    	dataY = [1,2,3,4,5]
    	workspace = CreateWorkspace(DataX=dataX, DataY=dataY)
    	info = workspace.detectorInfo()
    	self.assertEquals(info.size(), 0)

    """
    The following test cases test for returned objects to do with position
    and rotation.
    """

    def test_position(self):
        """ Test that the detector's position is returned. """
        info = self._ws.detectorInfo()
        self.assertEquals(type(info.position(0)), V3D)
        self.assertEquals(type(info.position(1)), V3D)

    def test_rotation(self):
        """ Test that the detector's rotation is returned. """
        info = self._ws.detectorInfo()
        self.assertEquals(type(info.rotation(0)), Quat)
        self.assertEquals(type(info.rotation(1)), Quat)


    """
    ---------------
    Iteration
    ---------------
    """

    def test_iteration_for_isMonitor(self):
        info = self._ws.detectorInfo()
        for detInfo in info:
            self.assertEquals(type(detInfo.isMonitor), bool)

    def test_iteration_for_isMasked(self):
        info = self._ws.detectorInfo()
        for detInfo in info:
            self.assertEquals(type(detInfo.isMasked), bool)

    def test_iteration_for_twoTheta(self):
        info = self._ws.detectorInfo()
        for detInfo in info:
            self.assertEquals(type(detInfo.twoTheta), float)

    def test_iteration_for_position(self):
        info = self._ws.detectorInfo()
        for detInfo in info:
            self.assertEquals(type(detInfo.position), V3D)

    def test_iteration_for_rotation(self):
        info = self._ws.detectorInfo()
        for detInfo in info:
            self.assertEquals(type(detInfo.rotation), Quat)


    """
    ----------------------------------------------------------------------------
    Extreme Tests
    ----------------------------------------------------------------------------

    The following test cases test around boundary cases for the exposed methods.
    """
    def test_isMonitor_extreme(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(OverflowError):
            info.isMonitor(-1)

    def test_isMasked_extreme(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(OverflowError):
            info.isMasked(-1)

    def test_twoTheta_extreme(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(OverflowError):
            info.twoTheta(-1)
        self.assertEquals(type(info.twoTheta(0)), float)
        self.assertEquals(type(info.twoTheta(1)), float)

    def test_position_extreme(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(OverflowError):
            info.position(-1)


    """
    ----------------------------------------------------------------------------
    Exceptional Tests
    ----------------------------------------------------------------------------

    Each of the tests below tries to pass invalid parameters to the exposed
    methods and expect an error to be thrown.
    """
    def test_size_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.size(0)

    def test_isMonitor_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.isMonitor("Error")
        with self.assertRaises(TypeError):
            info.isMonitor(10.0)

    def test_isMasked_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.isMasked("Error")
        with self.assertRaises(TypeError):
            info.isMasked(10.0)

    def test_setMasked_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.setMasked(0, "False")
        with self.assertRaises(TypeError):
            info.setMasked("False", True)

    def test_clearMaskFlags_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.clearMaskFlags("All")
        with self.assertRaises(TypeError):
            info.clearMaskFlags(1.10)

    def test_isEquivalent_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.isEquivalent("Hello")
        with self.assertRaises(TypeError):
            info.isEquivalent(11.1)

    def test_twoTheta_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.twoTheta("Zero")
        with self.assertRaises(TypeError):
            info.twoTheta(1.0)

    def test_position_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.position("Zero")
        with self.assertRaises(TypeError):
            info.position(0.0)

    def test_rotation_exceptional(self):
        info = self._ws.detectorInfo()
        with self.assertRaises(TypeError):
            info.rotation("Zero")
        with self.assertRaises(TypeError):
            info.rotation(0.0)

if __name__ == '__main__':
    unittest.main()
