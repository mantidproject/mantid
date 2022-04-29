# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantidqtinterfaces.dns_powder_tof.helpers.list_range_converters import (
    get_normation, list_to_multirange, list_to_range)


class list_range_convertersTest(unittest.TestCase):

    def test_list_to_range(self):
        testv = list_to_range([1, 2, 3])
        self.assertEqual(testv, '[1, 2, 3]')
        testv = list_to_range([1, 2, 3, 4])
        self.assertEqual(testv, '[*range(1, 5, 1)]')

    def test_list_to_multirange(self):
        testv = list_to_multirange([1, 2, 3])
        self.assertEqual(testv, '[1, 2, 3]')
        testv = list_to_multirange([1, 2, 3, 4, 5, 6])
        self.assertEqual(testv, 'range(1, 7, 1)')
        testv = list_to_multirange([1, 3, 5, 7, 9, 11, 12])
        self.assertEqual(testv, '[*range(1, 13, 2)] + [12]')
        testv = list_to_multirange(
            [1, 11, 12, 13, 14, 15, 16, 20, 21, 22, 23, 24, 25, 26])
        self.assertEqual(
            testv, '[1, 11] + [*range(12, 17, 1)]'
            ' + [*range(20, 27, 1)]')

    def test_get_normation(self):
        testv = get_normation({'norm_monitor': True})
        self.assertEqual(testv, 'monitor')
        testv = get_normation({'norm_monitor': False})
        self.assertEqual(testv, 'time')
        testv = get_normation({})
        self.assertEqual(testv, 'time')


if __name__ == '__main__':
    unittest.main()
