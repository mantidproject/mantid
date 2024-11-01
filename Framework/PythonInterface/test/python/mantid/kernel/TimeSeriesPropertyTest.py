# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from mantid.kernel import DateAndTime, BoolTimeSeriesProperty, FloatTimeSeriesProperty, Int64TimeSeriesProperty, StringTimeSeriesProperty
from testhelpers import run_algorithm


class TimeSeriesPropertyTest(unittest.TestCase):
    _test_ws = None
    _ntemp = 5
    _nframes = 6

    def setUp(self):
        if self._test_ws is not None:
            return
        alg = run_algorithm("CreateWorkspace", DataX=[1, 2, 3, 4, 5], DataY=[1, 2, 3, 4, 5], NSpec=1, child=True)
        ws = alg.getProperty("OutputWorkspace").value
        run = ws.run()

        start_time = DateAndTime("2008-12-18T17:58:38")
        nanosec = 1000000000
        # === Float type ===
        temp1 = FloatTimeSeriesProperty("TEMP1")
        tempvalue = -0.00161
        for i in range(self._ntemp):
            temp1.addValue(start_time + i * nanosec, tempvalue)
        run.addProperty(temp1.name, temp1, True)

        # === Int type ===
        raw_frames = Int64TimeSeriesProperty("raw_frames")
        values = [17, 1436, 2942, 4448, 5955, 7461]
        for value in values:
            raw_frames.addValue(start_time + i * nanosec, value)
        run.addProperty(raw_frames.name, raw_frames, True)

        # === String type ===
        icp_event = temp1 = StringTimeSeriesProperty("icp_event")
        values = [
            "CHANGE_PERIOD 1",
            "START_COLLECTION PERIOD 1 GF 0 RF 0 GUAH 0.000000",
            "BEGIN",
            "STOP_COLLECTION PERIOD 1 GF 1053 RF 1053 GUAH 0.000000 DUR 22",
        ]
        for value in values:
            icp_event.addValue(start_time + i * nanosec, value)
        run.addProperty(icp_event.name, icp_event, True)

        # === Boolean type ===
        period_1 = temp1 = BoolTimeSeriesProperty("period 1")
        values = [True]
        for value in values:
            period_1.addValue(start_time + i * nanosec, value)
        run.addProperty(period_1.name, period_1, True)

        self.__class__._test_ws = ws

    def test_time_series_double_can_be_extracted(self):
        log_series = self._test_ws.getRun()["TEMP1"]
        self._check_has_time_series_attributes(log_series)
        self.assertEqual(log_series.size(), self._ntemp)
        self.assertAlmostEqual(log_series.nthValue(0), -0.00161)
        # Check the dtype return value
        self.assertEqual(log_series.dtype(), "f")

    def test_time_series_int_can_be_extracted(self):
        log_series = self._test_ws.getRun()["raw_frames"]
        self._check_has_time_series_attributes(log_series)
        self.assertEqual(log_series.size(), self._nframes)
        self.assertEqual(log_series.nthValue(1), 1436)
        # Check the dtype return value
        self.assertEqual(log_series.dtype(), "i")

    def test_time_series_string_can_be_extracted(self):
        log_series = self._test_ws.getRun()["icp_event"]
        self._check_has_time_series_attributes(log_series, list)
        self.assertEqual(log_series.size(), 4)
        self.assertEqual(log_series.nthValue(0).strip(), "CHANGE_PERIOD 1")
        # Check the dtype return value
        self.assertEqual(log_series.dtype(), "S61")

    def test_time_series_bool_can_be_extracted(self):
        log_series = self._test_ws.getRun()["period 1"]
        self._check_has_time_series_attributes(log_series)
        self.assertEqual(log_series.size(), 1)
        # Check the dtype return value
        self.assertEqual(log_series.dtype(), "b")

    def _check_has_time_series_attributes(self, log, values_type=np.ndarray):
        self.assertTrue(hasattr(log, "value"))
        self.assertTrue(hasattr(log, "times"))
        self.assertTrue(hasattr(log, "getStatistics"))

        values = log.value
        self.assertTrue(isinstance(values, values_type))
        self.assertEqual(log.size(), len(values))

        # check the statistics
        stats = log.getStatistics()
        self.assertTrue(hasattr(stats, "mean"))
        self.assertTrue(hasattr(stats, "median"))
        self.assertTrue(hasattr(stats, "minimum"))
        self.assertTrue(hasattr(stats, "maximum"))
        self.assertTrue(hasattr(stats, "standard_deviation"))
        self.assertTrue(hasattr(stats, "time_mean"))
        self.assertTrue(hasattr(stats, "time_standard_deviation"))


if __name__ == "__main__":
    unittest.main()
