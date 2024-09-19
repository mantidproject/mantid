# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from SANS.sans.state.StateObjects.wavelength_interval import WavelengthInterval


class WavelengthIntervalTest(unittest.TestCase):
    def test_duplicates_removed(self):
        interval = WavelengthInterval()
        interval.selected_ranges = [(1, 2), (1, 2), (1, 1)]
        self.assertEqual(2, len(interval.selected_ranges))

    def test_sorts_wavelength_intervals_into_order(self):
        interval = WavelengthInterval()
        interval.selected_ranges = [(2, 4), (2, 6), (4, 6)]
        # Should sort the full interval into the front of the queue
        self.assertEqual([(2, 6), (2, 4), (4, 6)], interval.selected_ranges)

    def test_setting_full_range_inserts_into_selected(self):
        interval = WavelengthInterval()
        expected = (1.0, 2.0)
        interval.wavelength_full_range = expected
        self.assertEqual(expected, interval.wavelength_full_range)
        self.assertEqual([expected], interval.selected_ranges)


if __name__ == "__main__":
    unittest.main()
