# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import numpy as np
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_helpers import (
    get_hkl_float_array,
    angle_to_q,
)


class ElasticSCHelpersTest(unittest.TestCase):
    def test_get_hkl_float_array_basic(self):
        result = get_hkl_float_array("0.0,5,-3")
        self.assertTrue(np.array_equal(result, np.array([0.0, 5.0, -3.0])))

    def test_get_hkl_float_array_with_spaces_and_negatives(self):
        result = get_hkl_float_array(" -1 , 0 , 2.5 ")
        self.assertTrue(np.array_equal(result, np.array([-1.0, 0.0, 2.5])))

    def test_angle_to_q(self):
        two_theta = np.array([90, 60])
        omega = np.array([0, 60])
        wavelength = 2.0 * np.pi
        qx, qy = angle_to_q(two_theta, omega, wavelength)
        expected_qx = [1, -0.5]
        expected_qy = [-1, -np.sqrt(3) / 2]
        self.assertTrue(np.allclose(qx, expected_qx))
        self.assertTrue(np.allclose(qy, expected_qy))


if __name__ == "__main__":
    unittest.main()
