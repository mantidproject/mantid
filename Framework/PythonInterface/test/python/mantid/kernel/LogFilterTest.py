# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import LogFilter, FloatTimeSeriesProperty, BoolTimeSeriesProperty


class LogFilterTest(unittest.TestCase):
    def test_addFilter_filters_log(self):
        height_log = FloatTimeSeriesProperty("height_log")
        height_log.addValue("2008-Jun-17 11:10:44", -0.86526)
        height_log.addValue("2008-Jun-17 11:10:45", -1.17843)
        height_log.addValue("2008-Jun-17 11:10:47", -1.27995)
        height_log.addValue("2008-Jun-17 11:20:15", -1.38216)
        height_log.addValue("2008-Jun-17 11:20:16", -1.87435)
        height_log.addValue("2008-Jun-17 11:20:17", -2.70547)
        height_log.addValue("2008-Jun-17 11:20:19", -2.99125)
        height_log.addValue("2008-Jun-17 11:20:20", -3)
        height_log.addValue("2008-Jun-17 11:20:27", -2.98519)
        height_log.addValue("2008-Jun-17 11:20:29", -2.68904)

        period_log = BoolTimeSeriesProperty("period 7")
        period_log.addValue("2008-Jun-17 11:11:13", False)
        period_log.addValue("2008-Jun-17 11:11:13", False)
        period_log.addValue("2008-Jun-17 11:11:18", False)
        period_log.addValue("2008-Jun-17 11:11:30", False)
        period_log.addValue("2008-Jun-17 11:11:42", False)
        period_log.addValue("2008-Jun-17 11:11:52", False)
        period_log.addValue("2008-Jun-17 11:12:01", False)
        period_log.addValue("2008-Jun-17 11:12:11", False)
        period_log.addValue("2008-Jun-17 11:12:21", True)
        period_log.addValue("2008-Jun-17 11:12:32", False)

        self.assertEqual(height_log.size(), 10)
        filter = LogFilter(height_log)
        filter.addFilter(period_log)
        filtered = filter.data()
        self.assertEqual(filtered.size(), 1)


if __name__ == "__main__":
    unittest.main()
