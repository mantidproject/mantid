# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import BoolTimeSeriesProperty, FloatTimeSeriesProperty, FloatFilteredTimeSeriesProperty


class FilteredTimeSeriesPropertyTest(unittest.TestCase):
    _source = None
    _filter = None

    def setUp(self):
        if self.__class__._source is not None:
            return
        height = FloatTimeSeriesProperty("height")
        height.addValue("2007-11-30T16:17:00", 1)
        height.addValue("2007-11-30T16:17:10", 2)
        height.addValue("2007-11-30T16:17:20", 3)
        height.addValue("2007-11-30T16:17:30", 4)
        height.addValue("2007-11-30T16:17:40", 5)

        filter = BoolTimeSeriesProperty("filter")
        filter.addValue("2007-11-30T16:16:50", False)
        filter.addValue("2007-11-30T16:17:25", True)
        filter.addValue("2007-11-30T16:17:39", False)

        self.__class__._source = height
        self.__class__._filter = filter

    def test_constructor_filters_source_series(self):
        filtered = FloatFilteredTimeSeriesProperty(self._source, self._filter)
        self.assertEqual(filtered.size(), 2)

    def test_unfiltered_returns_source_property(self):
        filtered = FloatFilteredTimeSeriesProperty(self._source, self._filter)
        unfiltered = filtered.unfiltered()

        self.assertEqual(self._source.size(), unfiltered.size())

    def test_time_average_value(self):
        filter = BoolTimeSeriesProperty("filter")
        filter.addValue("2007-11-30T16:17:20", True)
        filter.addValue("2007-11-30T16:17:30", False)

        filtered_tsp = FloatFilteredTimeSeriesProperty(self._source, filter)
        self.assertAlmostEqual(filtered_tsp.timeAverageValue(), 3, 10)


if __name__ == "__main__":
    unittest.main()
