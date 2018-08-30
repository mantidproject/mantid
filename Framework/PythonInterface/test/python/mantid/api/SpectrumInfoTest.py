from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.api import SpectrumDefinition

class SpectrumInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        """ Set up code. """
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
        """ Check that the number of spectra we initailly created
            is the same as in memory. """
        info = self._ws.spectrumInfo()
        self.assertEquals(len(info), 2)

    def test_size(self):
        """ Check that the number of spectra we initailly created
            is the same as in memory. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.size(), 2)

    def test_isMonitor(self):
        """ Check if a monitor is present. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.isMonitor(1), False)

    def test_isMasked(self):
        """ Check if the detector is masked. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.isMasked(1), False)

    def test_setMaskedTrue(self):
        """ Test that the detector's masking can be set to True. """
        info = self._ws.spectrumInfo()
        info.setMasked(1, True)
        self.assertEquals(info.isMasked(1), True)

    def test_setMaskedFalse(self):
        """ Test that the detector's masking can be set to False. """
        info = self._ws.spectrumInfo()
        info.setMasked(1, False)
        self.assertEquals(info.isMasked(1), False)

    def test_twoTheta(self):
        """ See if the returned value is a double (float in Python). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.twoTheta(1)), float)

    def test_signedTwoTheta(self):
        """ See if the returned value is a double (float in Python). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.signedTwoTheta(1)), float)

    def test_l1(self):
        """ Check if a distance is returned (source to sample). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.l1()), float)

    def test_l2(self):
        """ Check if a distance is returned (sample to spectrum). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.l2(1)), float)

    def test_hasDetectors(self):
        """ Check to see which spectrum has detectors. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.hasDetectors(0), False)
        self.assertEquals(info.hasDetectors(1), True)

    def test_hasUniqueDetector(self):
        """ Test if the spectra have unique detectors. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.hasUniqueDetector(0), False)
        self.assertEquals(info.hasUniqueDetector(1), True)

    """
    The following are test cases test for returned V3D objects. The objects
    represent a vector in 3 dimensions.
    """

    def test_position(self):
        """ Test that the spectrum's position is returned. """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.position(1)), V3D)

    def test_sourcePosition(self):
        """ Test that the source's position is returned. """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.sourcePosition()), V3D)

    def test_samplePosition(self):
        """ Test that the sample's position is returned. """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.samplePosition()), V3D)

    """
    ---------------
    Iteration
    ---------------
    """

    def test_iteration_for_isMonitor(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.isMonitor), bool)

    def test_iteration_for_isMasked(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.isMasked), bool)

    def test_iteration_for_twoTheta(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.twoTheta), float)

    def test_iteration_for_signedTwoTheta(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.signedTwoTheta), float)

    def test_iteration_for_l2(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.l2), float)

    def test_iteration_for_hasUniqueDetector(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            self.assertEquals(type(specInfo.hasUniqueDetector), bool)

    def test_iteration_for_spectrumDefinition(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.spectrumDefinition), SpectrumDefinition)

    def test_iteration_for_position(self):
        info = self._ws.spectrumInfo()
        for specInfo in info:
            if specInfo.hasUniqueDetector:
                self.assertEquals(type(specInfo.position), V3D)

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
        """ Check the size of the SpectrumDefinition
        (i.e. number of detectors) """
        info = self._ws.spectrumInfo()
        spectrumDefinitionOne = info.getSpectrumDefinition(0)
        spectrumDefinitionTwo = info.getSpectrumDefinition(1)
        self.assertEquals(spectrumDefinitionOne.size(), 0)
        self.assertEquals(spectrumDefinitionTwo.size(), 1)

    def test_spectrumDefintionAdd(self):
        """ Add a pair of detector index and time index """
        info = self._ws.spectrumInfo()
        spectrumDefinition = info.getSpectrumDefinition(1)
        spectrumDefinition.add(1, 1)
        self.assertEquals(spectrumDefinition.size(), 2)

    def test_spectrumDefintionEquals(self):
        """ Check the equality of the SpectrumDefintions """
        info = self._ws.spectrumInfo()
        spectrumDefinition = info.getSpectrumDefinition(0)
        # Check equality with equals() and == to make sure same result is given
        self.assertTrue(spectrumDefinition.equals(spectrumDefinition))
        self.assertTrue(spectrumDefinition == spectrumDefinition)

    def test_spectrumDefintionNotEquals(self):
        """ Check the equality of the SpectrumDefintions """
        info = self._ws.spectrumInfo()
        spectrumDefinitionOne = info.getSpectrumDefinition(0)
        spectrumDefinitionTwo = info.getSpectrumDefinition(1)
        # Check inequality with not (by negating equals())
        # and != to make sure same result is given
        self.assertTrue(not spectrumDefinitionOne.equals(spectrumDefinitionTwo))
        self.assertTrue(spectrumDefinitionOne != spectrumDefinitionTwo)

    def test_spectrumDefintionGet(self):
        """ See if indexing works """
        info = self._ws.spectrumInfo()
        spectrumDefinition = info.getSpectrumDefinition(1)
        self.assertEquals(spectrumDefinition[0], (1, 0))

if __name__ == '__main__':
    unittest.main()
