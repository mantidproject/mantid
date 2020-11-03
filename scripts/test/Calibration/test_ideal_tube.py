# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from numpy.testing import assert_allclose
import unittest
from Calibration.ideal_tube import IdealTube


class TestIdealTube(unittest.TestCase):
    def setUp(self):
        tube_length = 0.003278125 * 256  # assume a tube of 256 pixels, each about 3 mm tall
        self.peaks_count = 16  # assume we calibrate the tube with 16 peaks
        # partition the tube into 16 chunks and output the middle-position of each chunk
        self.peak_centers = np.linspace(0, tube_length, 2 * self.peaks_count, endpoint=False)[1::2]

    def test_init(self):
        self.assertTrue(IdealTube())

    def test_setArray(self):
        tube = IdealTube()
        tube.setArray(self.peak_centers)
        assert_allclose(tube.positions, self.peak_centers, atol=1e-6)

    def test_getArray(self):
        tube = IdealTube()
        tube.setArray(self.peak_centers)
        self.assertTrue(isinstance(tube.getArray(), np.ndarray))


if __name__ == '__main__':
    unittest.main()
