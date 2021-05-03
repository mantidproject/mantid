# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadWAND
import unittest


class LoadWANDTest(unittest.TestCase):
    def test(self):
        ws = LoadWAND('HB2C_7000.nxs.h5', Grouping='2x2')
        self.assertTrue(ws)
        self.assertEqual(ws.blocksize(), 1)
        self.assertEqual(ws.getNumberHistograms(), 1966080 // 4)
        self.assertEqual(ws.readY(257775), 4)
        self.assertEqual(ws.run().getProtonCharge(), 907880)
        self.assertAlmostEqual(ws.run().getGoniometer().getEulerAngles()[0], -142.6)
        self.assertAlmostEqual(ws.run().getLogData('duration').value, 40.05)

        # Check masking
        self.assertTrue(ws.detectorInfo().isMasked(0))
        self.assertTrue(ws.detectorInfo().isMasked(1))
        self.assertFalse(ws.detectorInfo().isMasked(2))
        self.assertTrue(ws.detectorInfo().isMasked(512))
        self.assertTrue(ws.detectorInfo().isMasked(480 * 512 * 8 - 256))
        self.assertFalse(ws.detectorInfo().isMasked(480 * 512 * 8 - 256 - 512 * 6))

        ws.delete()


if __name__ == '__main__':
    unittest.main()
