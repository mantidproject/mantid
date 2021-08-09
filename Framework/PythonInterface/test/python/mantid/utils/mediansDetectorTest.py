# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.utils.nomad import determine_tubes_threshold


class DetectorMediansTest(unittest.TestCase):

    def test_determine_tubes_thresholds(self):
        """

        Returns
        -------

        """
        lower_pixel = 0.9
        high_pixel = 1.2

        test_data = self._generate_test_data()

        some_returns = determine_tubes_threshold()
        assert some_returns

    @staticmethod
    def _generate_test_data():
        """

        Returns
        -------

        """
        # 16 pixels per tube
        # 2  tubes  per n-pack
        tube_size = 12
        pack_size = 2

        # Test data: 3 2-packs for m, m1 and m2
        test_data = np.zeros(shape=(3 * pack_size, tube_size), dtype='float')

        full_collimated = np.zeros((tube_size,), dtype='float')
        full_collimated[0::2] = np.arange(1, tube_size//2 + 1)
        full_collimated[1::2] = np.arange(tube_size, tube_size//2, -1)

        # not collimated (0, 1) and fully collimated (2, 3)
        test_data[0] = full_collimated
        test_data[1] = np.flip(full_collimated, axis=0)
        test_data[2] = full_collimated
        test_data[3] = np.flip(full_collimated, axis=0)
        # half collimated
        test_data[4] = np.arange(1, tube_size + 1)
        test_data[5] = np.arange(tube_size, 0, -1)

        # Intentionally set the maximum value to a very large number to make
        # difference between average and median
        test_data[0, 1] *= 10
        test_data[1, tube_size - 2] *= 10
        test_data[2, 1] *= 10
        test_data[3, tube_size - 2] *= 12
        test_data[4, tube_size - 1] *= 10
        test_data[5, 0] *= 12

        print(f'[DEBUG] Test 3 x 2 pack:\n{test_data}')

        return test_data.flatten()


if __name__ == '__main__':
    unittest.main()
