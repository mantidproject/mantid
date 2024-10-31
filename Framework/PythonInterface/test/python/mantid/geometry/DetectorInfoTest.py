# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.kernel import Quat
from mantid.simpleapi import CreateWorkspace
import numpy as np


class DetectorInfoTest(unittest.TestCase):
    _ws = None

    def setUp(self):
        """Setup Workspace to use"""
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False)  # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    """
    ----------------------------------------------------------------------------
    Normal Tests
    ----------------------------------------------------------------------------

    The following test cases test normal usage of the exposed methods.
    """

    def test_len(self):
        """Test return value for number of detectors"""
        info = self._ws.detectorInfo()
        self.assertEqual(len(info), 2)

    def test_size(self):
        """Test return value for number of detectors"""
        info = self._ws.detectorInfo()
        self.assertEqual(info.size(), 2)

    def test_indexOf(self):
        """Check if detector is a monitor"""
        info = self._ws.detectorInfo()
        print(str(info.detectorIDs()))
        self.assertEqual(info.indexOf(1), 0)
        self.assertEqual(info.indexOf(2), 1)

        with self.assertRaises(IndexError):
            info.indexOf(3)

    def test_isMonitor(self):
        """Check if detector is a monitor"""
        info = self._ws.detectorInfo()
        self.assertEqual(info.isMonitor(0), False)
        self.assertEqual(info.isMonitor(1), False)

    def test_isMasked(self):
        """Check masking of detector"""
        info = self._ws.detectorInfo()
        self.assertEqual(info.isMasked(0), False)
        self.assertEqual(info.isMasked(1), False)

    def test_setMasked(self):
        """Test that the detector's masking can be set to True."""
        info = self._ws.detectorInfo()
        info.setMasked(0, True)
        self.assertTrue(info.isMasked(0))

    def test_clearMaskFlags(self):
        """Test that the detector's masking can be cleared."""
        info = self._ws.detectorInfo()
        info.setMasked(0, True)
        self.assertTrue(info.isMasked(0))
        info.clearMaskFlags()
        self.assertFalse(info.isMasked(0))

    def test_isEquivalent(self):
        """Check equality of detectors"""
        info = self._ws.detectorInfo()
        self.assertTrue(info.isEquivalent(info))
        self.assertEqual(info, info)

    def test_twoTheta(self):
        """See if the returned value is a double (float in Python)."""
        info = self._ws.detectorInfo()
        self.assertEqual(type(info.twoTheta(0)), float)

    def test_azimuthal(self):
        """See if the returned value is a double (float in Python)."""
        info = self._ws.detectorInfo()
        self.assertEqual(type(info.azimuthal(0)), float)
        for i in range(info.size()):
            x, y, _ = info.position(i)
            self.assertEqual(np.arctan2(y, x), info.azimuthal(i))

    def test_createWorkspaceAndDetectorInfo(self):
        """Try to create a workspace and see if DetectorInfo object
        is accessable"""
        dataX = [1, 2, 3, 4, 5]
        dataY = [1, 2, 3, 4, 5]
        workspace = CreateWorkspace(DataX=dataX, DataY=dataY)
        info = workspace.detectorInfo()
        self.assertEqual(info.size(), 0)

    def test_detectorIds(self):
        info = self._ws.detectorInfo()
        ids = info.detectorIDs()
        # Should be read-only
        self.assertFalse(ids.flags.writeable)
        for a, b in zip(ids, [1, 2]):
            self.assertEqual(a, b)

    def test_position(self):
        """Test that the detector's position is returned."""
        info = self._ws.detectorInfo()
        self.assertEqual(type(info.position(0)), V3D)
        self.assertEqual(type(info.position(1)), V3D)

    def test_rotation(self):
        """Test that the detector's rotation is returned."""
        info = self._ws.detectorInfo()
        self.assertEqual(type(info.rotation(0)), Quat)
        self.assertEqual(type(info.rotation(1)), Quat)

    def test_l2(self):
        det_info = self._ws.detectorInfo()
        sample_pos = self._ws.componentInfo().samplePosition()
        l2_calc = det_info.position(0).distance(sample_pos)
        self.assertEqual(det_info.l2(0), l2_calc)

    def test_l1(self):
        source_pos = self._ws.componentInfo().sourcePosition()
        sample_pos = self._ws.componentInfo().samplePosition()
        l1_calc = sample_pos.distance(source_pos)
        det_info = self._ws.detectorInfo()
        self.assertEqual(det_info.l1(), l1_calc)

    """
    ---------------
    Iteration
    ---------------
    """

    def test_basic_iteration(self):
        info = self._ws.detectorInfo()
        expected_iterations = len(info)
        actual_iterations = len(list(iter(info)))
        self.assertEqual(expected_iterations, actual_iterations)
        it = iter(info)
        self.assertEqual(next(it).index, 0)
        self.assertEqual(next(it).index, 1)

    def test_iterator_for_monitors(self):
        info = self._ws.detectorInfo()
        # check no monitors in instrument
        for item in info:
            self.assertFalse(item.isMonitor)

    def test_iterator_for_nothing_masked(self):
        info = self._ws.detectorInfo()
        # nothing should be masked
        for item in info:
            self.assertFalse(item.isMasked)

    def test_iterator_for_one_item_masked(self):
        info = self._ws.detectorInfo()
        it = iter(info)
        item = next(it)
        item.setMasked(True)
        self.assertTrue(item.isMasked)
        item.setMasked(False)
        self.assertFalse(item.isMasked)

    def test_iteration_for_position(self):
        info = self._ws.detectorInfo()
        lastY = None
        for i, item in enumerate(info):
            pos = item.position
            # See test helper for position construction
            self.assertAlmostEqual(pos.X(), 0)
            self.assertAlmostEqual(pos.Z(), 5)
            if lastY:
                self.assertGreater(pos.Y(), lastY)
            lastY = pos.Y()

    def test_iterator_for_l2(self):
        info = self._ws.detectorInfo()
        for item in info:
            self.assertGreater(item.l2, 0)

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
        self.assertEqual(type(info.twoTheta(0)), float)
        self.assertEqual(type(info.twoTheta(1)), float)

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


if __name__ == "__main__":
    unittest.main()
