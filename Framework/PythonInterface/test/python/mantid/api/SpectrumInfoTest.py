# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
import numpy as np


class SpectrumInfoTest(unittest.TestCase):
    _ws = None

    def setUp(self):
        """Set up code."""
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(3, 1, False)  # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    """
    ----------------------------------------------------------------------------
    Normal Tests
    ----------------------------------------------------------------------------
    The following test cases test normal usage of the exposed methods.
    """

    def test_len(self):
        """Check that the number of spectra we initially created
        is the same as in memory."""
        info = self._ws.spectrumInfo()
        self.assertEqual(len(info), 3)

    def test_size(self):
        """Check that the number of spectra we initially created
        is the same as in memory."""
        info = self._ws.spectrumInfo()
        self.assertEqual(info.size(), 3)

    def test_detectorCount(self):
        """Check total detector count"""
        info = self._ws.spectrumInfo()
        # One detector cleared. So not counted.
        self.assertEqual(info.detectorCount(), 2)

    def test_isMonitor(self):
        """Check if a monitor is present."""
        info = self._ws.spectrumInfo()
        self.assertEqual(info.isMonitor(1), False)

    def test_isMasked(self):
        """Check if the detector is masked."""
        info = self._ws.spectrumInfo()
        self.assertEqual(info.isMasked(1), False)

    def test_setMaskedTrue(self):
        """Test that the detector's masking can be set to True."""
        info = self._ws.spectrumInfo()
        info.setMasked(1, True)
        self.assertEqual(info.isMasked(1), True)

    def test_setMaskedFalse(self):
        """Test that the detector's masking can be set to False."""
        info = self._ws.spectrumInfo()
        info.setMasked(1, False)
        self.assertEqual(info.isMasked(1), False)

    def test_twoTheta(self):
        """See if the returned value is a double (float in Python)."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.twoTheta(1)), float)

    def test_angles_with_observer(self):
        info = self._ws.spectrumInfo()
        pos = np.array(info.position(1))
        o, a, u = np.array([0.1, -0.2, 0.3]), np.array([0.0, 0.1, 1.0]), np.array([0.3, 1.0, 0.0])
        v = pos - o
        expected = np.arccos(np.dot(v, a) / (np.linalg.norm(v) * np.linalg.norm(a)))
        sign = -1.0 if np.dot(np.cross(a, u), np.cross(a, v)) < 0 else 1.0
        expected_phi = np.arctan2(pos[1], pos[0])
        observer, axis, up = V3D(*o), V3D(*a), V3D(*u)
        self.assertAlmostEqual(info.twoTheta(1, observer, axis), expected, places=12)
        self.assertAlmostEqual(info.signedTwoTheta(1, observer, axis, up), sign * expected, places=12)
        self.assertAlmostEqual(info.phi(1), expected_phi, places=12)
        expected_offset = -(np.pi + expected_phi) if expected_phi < 0 else np.pi - expected_phi
        self.assertAlmostEqual(info.phiOffset(1, np.pi), expected_offset, places=12)

    def test_twoTheta_defaults(self):
        info = self._ws.spectrumInfo()
        index = 1
        sample = info.samplePosition()
        beam = sample - info.sourcePosition()
        observer = V3D(0.1, -0.2, 0.3)
        axis = V3D(0.0, 0.1, 1.0)
        up = V3D(0.3, 1.0, 0.0)
        self.assertEqual(info.twoTheta(index, sample, beam), info.twoTheta(index))
        self.assertEqual(info.twoTheta(index, observer=observer), info.twoTheta(index, observer, beam))
        self.assertEqual(info.twoTheta(index, axis=axis), info.twoTheta(index, sample, axis))
        self.assertEqual(info.signedTwoTheta(index, sample, beam), info.signedTwoTheta(index))
        self.assertEqual(info.signedTwoTheta(index, observer=observer), info.signedTwoTheta(index, observer, beam))
        self.assertEqual(info.signedTwoTheta(index, axis=axis), info.signedTwoTheta(index, sample, axis))
        self.assertEqual(info.signedTwoTheta(index, instrumentUp=up), info.signedTwoTheta(index, sample, beam, up))

    def test_geogAngles(self):
        """See if the returned value is a double (float in Python)."""
        info = self._ws.spectrumInfo()
        lat, lon = info.geographicalAngles(1)
        self.assertEqual(type(lat), float)
        self.assertEqual(type(lon), float)

    def test_signedTwoTheta(self):
        """See if the returned value is a double (float in Python)."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.signedTwoTheta(1)), float)

    def test_l1(self):
        """Check if a distance is returned (source to sample)."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.l1()), float)

    def test_l2(self):
        """Check if a distance is returned (sample to spectrum)."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.l2(1)), float)

    def test_hasDetectors(self):
        """Check to see which spectrum has detectors."""
        info = self._ws.spectrumInfo()
        self.assertEqual(info.hasDetectors(0), False)
        self.assertEqual(info.hasDetectors(1), True)

    def test_hasUniqueDetector(self):
        """Test if the spectra have unique detectors."""
        info = self._ws.spectrumInfo()
        self.assertEqual(info.hasUniqueDetector(0), False)
        self.assertEqual(info.hasUniqueDetector(1), True)

    """
    The following are test cases test for returned V3D objects. The objects
    represent a vector in 3 dimensions.
    """

    def test_position(self):
        """Test that the spectrum's position is returned."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.position(1)), V3D)

    def test_sourcePosition(self):
        """Test that the source's position is returned."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.sourcePosition()), V3D)

    def test_samplePosition(self):
        """Test that the sample's position is returned."""
        info = self._ws.spectrumInfo()
        self.assertEqual(type(info.samplePosition()), V3D)

    """
    ---------------
    Iteration
    ---------------
    """

    def test_basic_iteration(self):
        info = self._ws.spectrumInfo()
        expected_iterations = len(info)
        actual_iterations = len(list(iter(info)))
        self.assertEqual(expected_iterations, actual_iterations)

    def test_iterator_for_monitors(self):
        info = self._ws.spectrumInfo()
        # check no monitors in instrument
        it = iter(info)
        next(it)  # skip first as detectors cleared
        for item in it:
            self.assertFalse(item.isMonitor)

    def test_iterator_for_masked(self):
        info = self._ws.spectrumInfo()
        # nothing should be masked
        it = iter(info)
        next(it)  # skip first as detectors cleared
        for item in it:
            self.assertFalse(item.isMasked)

    def test_iterator_for_setting_masked(self):
        info = self._ws.spectrumInfo()
        # nothing should be masked
        it = iter(info)
        next(it)  # skip first as detectors cleared
        for item in it:
            # mask and check
            item.setMasked(True)
            self.assertTrue(item.isMasked)

    def test_iteration_for_position(self):
        info = self._ws.spectrumInfo()
        lastY = None
        it = iter(info)
        next(it)  # skip first as detectors cleared
        for i, item in enumerate(it):
            pos = item.position
            # See test helper for position construction
            self.assertAlmostEqual(pos.X(), 0)
            self.assertAlmostEqual(pos.Z(), 5)
            if lastY:
                self.assertGreater(pos.Y(), lastY)
            lastY = pos.Y()

    """
    ----------------------------------------------------------------------------
    Extreme Tests
    ----------------------------------------------------------------------------
    The following test cases test around boundary cases for the exposed methods.
    """

    def test_isMonitor_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.isMonitor(-1)

    def test_isMasked_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.isMasked(-1)

    def test_setMasked_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.setMasked(-1, True)

    def test_twoTheta_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.twoTheta(-1)

    def test_signedTwoTheta_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.signedTwoTheta(-1)

    def test_l2_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.l2(-1)

    def test_hasUniqueDetector_extreme(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(OverflowError):
            info.hasUniqueDetector(-1)

    def test_position_extreme(self):
        info = self._ws.spectrumInfo()
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
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.size(0)

    def test_isMonitor_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.isMonitor("Error")
        with self.assertRaises(TypeError):
            info.isMonitor(10.0)

    def test_isMasked_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.isMasked("Error")
        with self.assertRaises(TypeError):
            info.isMasked(10.0)

    def test_setMasked_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.setMasked("Error", True)
        with self.assertRaises(TypeError):
            info.isMasked(10.0, True)
        with self.assertRaises(TypeError):
            info.setMasked(0, "True")
        with self.assertRaises(TypeError):
            info.isMasked(0, 1.0)

    def test_twoTheta_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.twoTheta("Zero")
        with self.assertRaises(TypeError):
            info.twoTheta(1.0)

    def test_signedTwoTheta_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.signedTwoTheta("Zero")
        with self.assertRaises(TypeError):
            info.signedTwoTheta(1.0)

    def test_l1_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.l1("One")

    def test_l2_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.l2("One")
        with self.assertRaises(TypeError):
            info.l2(10.0)

    def test_l1_value(self):
        spec_info = self._ws.spectrumInfo()
        comp_info = self._ws.componentInfo()
        specL1 = spec_info.l1()
        specL2 = comp_info.sourcePosition().distance(comp_info.samplePosition())
        self.assertEqual(specL1, specL2)

    def test_hasDetectors_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.hasDetectors("Zero")

    def test_hasUniqueDetector_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.hasUniqueDetector("Zero")
        with self.assertRaises(TypeError):
            info.hasUniqueDetector(0.0)

    def test_position_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.position("Zero")
        with self.assertRaises(TypeError):
            info.position(0.0)

    def test_sourcePosition_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.sourcePosition("Zero")
        with self.assertRaises(TypeError):
            info.sourcePosition(0.0)

    def test_samplePosition_exceptional(self):
        info = self._ws.spectrumInfo()
        with self.assertRaises(TypeError):
            info.samplePosition("Zero")
        with self.assertRaises(TypeError):
            info.samplePosition(0.0)

    """
    ----------------------------------------------------------------------------
    SpectrumDefinition Tests
    ----------------------------------------------------------------------------

    The following are test cases for the exposed SpectrumDefintion methods.
    """

    def test_spectrumDefintionSize(self):
        """Check the size of the SpectrumDefinition
        (i.e. number of detectors)"""
        info = self._ws.spectrumInfo()
        spectrumDefinitionOne = info.getSpectrumDefinition(0)
        spectrumDefinitionTwo = info.getSpectrumDefinition(1)
        self.assertEqual(spectrumDefinitionOne.size(), 0)
        self.assertEqual(spectrumDefinitionTwo.size(), 1)

    def test_spectrumDefintionAdd(self):
        """Add a pair of detector index and time index"""
        info = self._ws.spectrumInfo()
        spectrumDefinition = info.getSpectrumDefinition(1)
        spectrumDefinition.add(1, 1)
        self.assertEqual(spectrumDefinition.size(), 2)

    def test_spectrumDefintionEquals(self):
        """Check the equality of the SpectrumDefintions"""
        info = self._ws.spectrumInfo()
        spectrumDefinition = info.getSpectrumDefinition(0)
        # Check equality with equals() and == to make sure same result is given
        self.assertTrue(spectrumDefinition.equals(spectrumDefinition))
        self.assertEqual(spectrumDefinition, spectrumDefinition)

    def test_spectrumDefintionNotEquals(self):
        """Check the equality of the SpectrumDefintions"""
        info = self._ws.spectrumInfo()
        spectrumDefinitionOne = info.getSpectrumDefinition(0)
        spectrumDefinitionTwo = info.getSpectrumDefinition(1)
        # Check inequality with not (by negating equals())
        # and != to make sure same result is given
        self.assertTrue(not spectrumDefinitionOne.equals(spectrumDefinitionTwo))
        self.assertNotEqual(spectrumDefinitionOne, spectrumDefinitionTwo)

    def test_spectrumDefintionGet(self):
        """See if indexing works"""
        info = self._ws.spectrumInfo()
        spectrumDefinition = info.getSpectrumDefinition(1)
        self.assertEqual(spectrumDefinition[0], (1, 0))


if __name__ == "__main__":
    unittest.main()
