# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import HB3APredictPeaks, HB3AAdjustSampleNorm


class HB3APredictPeaksTest(unittest.TestCase):
    _tolerance = 1.0e-7

    def testPredictPeaks(self):
        data = HB3AAdjustSampleNorm("HB3A_data.nxs")
        peaks = HB3APredictPeaks(data)
        self.assertEqual(peaks.getNumberPeaks(), 1)
        peak0 = peaks.getPeak(0)
        self.assertEqual(peak0.getH(), 0)
        self.assertEqual(peak0.getK(), 0)
        self.assertEqual(peak0.getL(), 6)
        self.assertEqual(peak0.getRunNumber(), 9)
        self.assertAlmostEqual(peak0.getWavelength(), 1.008)
        q_sample = peak0.getQSampleFrame()
        self.assertAlmostEqual(q_sample[0], -0.401367, places=5)
        self.assertAlmostEqual(q_sample[1], 1.71447, places=5)
        self.assertAlmostEqual(q_sample[2], 2.29083, places=5)


if __name__ == '__main__':
    unittest.main()
