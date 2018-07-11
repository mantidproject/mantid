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

    def test_hasDetectors(self):
        """ Check to see which spectrum has detectors. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.hasDetectors(0), False)
        self.assertEquals(info.hasDetectors(1), True)

    def test_isMasked(self):
        """ Check if the detector is masked. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.isMasked(1), False)

    def test_isMonitor(self):
        """ Check if a monitor is present. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.isMonitor(1), False)

    def test_twoTheta(self):
        """ See if the returned value is a double (float in Python). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.twoTheta(1)), float)

    def test_signedTwoTheta(self):
        """ See if the returned value is a double (float in Python). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.signedTwoTheta(1)), float)

    def test_size(self):
        """ Check that the number of spectra we initailly created
            is the same as in memory. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.size(), 2)

    def test_hasUniqueDetector(self):
        """ Test if the spectra have unique detectors. """
        info = self._ws.spectrumInfo()
        self.assertEquals(info.hasUniqueDetector(0), False)
        self.assertEquals(info.hasUniqueDetector(1), True)

    def test_l1(self):
        """ Check if a distance is returned (source to sample). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.l1()), float)

    def test_l2(self):
        """ Check if a distance is returned (sample to spectrum). """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.l2(1)), float)

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
    The following are test cases are SpectrumDefintion methods.
    """

    def test_spectrumDefinitionCall(self):
        """ Get the SpectrumDefintion """
        info = self._ws.spectrumInfo()
        self.assertEquals(type(info.spectrumDefinition(1)), SpectrumDefinition)

    def test_spectrumDefintionSize(self):
        """ Check the size of the SpectrumDefinition
        (i.e. number of detectors) """
        info = self._ws.spectrumInfo()
        spectrumDef = info.spectrumDefinition(1)
        self.assertEquals(spectrumDef.size(), 1)

    def test_spectrumDefintionAdd(self):
        """ Add a pair of detector index and time index """
        info = self._ws.spectrumInfo()
        spectrumDef = info.spectrumDefinition(1)
        spectrumDef.add(1, 1)
        self.assertEquals(spectrumDef.size(), 2)

    def test_spectrumDefintionEquals(self):
        """ Check the equality of the SpectrumDefintion """
        info = self._ws.spectrumInfo()
        spectrumDef = info.spectrumDefinition(1)
        self.assertTrue(spectrumDef.equals(spectrumDef))

    def test_spectrumDefintionGet(self):
        """ See if indexing works """
        info = self._ws.spectrumInfo()
        spectrumDef = info.spectrumDefinition(1)
        self.assertEquals(spectrumDef.get(0), (1, 0))

if __name__ == '__main__':
    unittest.main()
