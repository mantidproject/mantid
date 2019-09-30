# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
from mantid.simpleapi import LoadWAND
import unittest


class LoadWANDTest(unittest.TestCase):

    def test(self):
        ws = LoadWAND('HB2C_7000.nxs.h5', Grouping='2x2')
        self.assertTrue(ws)
        self.assertEqual(ws.blocksize(), 1)
        self.assertEqual(ws.getNumberHistograms(), 1966080//4)
        self.assertEqual(ws.readY(257775), 4)
        self.assertEqual(ws.run().getProtonCharge(), 907880)
        self.assertAlmostEqual(ws.run().getGoniometer().getEulerAngles()[0], -142.6)
        self.assertEqual(ws.run().getLogData('Wavelength').value, 1.488)
        self.assertAlmostEqual(ws.run().getLogData('Ei').value, 36.94619794)

        # Check masking
        self.assertTrue(ws.detectorInfo().isMasked(0))
        self.assertTrue(ws.detectorInfo().isMasked(1))
        self.assertFalse(ws.detectorInfo().isMasked(2))
        self.assertTrue(ws.detectorInfo().isMasked(512))
        self.assertTrue(ws.detectorInfo().isMasked(480*512*8-256))
        self.assertFalse(ws.detectorInfo().isMasked(480*512*8-256-512*6))

        # Check x dimension
        x=ws.getXDimension()
        self.assertEqual(x.name, 'Wavelength')
        self.assertEqual(x.getNBins(), 1)
        self.assertEqual(x.getNBoundaries(), 2)
        self.assertAlmostEqual(x.getMinimum(), 1.487)
        self.assertAlmostEqual(x.getMaximum(), 1.489)

        ws.delete()


if __name__ == '__main__':
    unittest.main()
