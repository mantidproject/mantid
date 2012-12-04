import unittest
from testhelpers import run_algorithm

class TimeSeriesPropertyTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            alg = run_algorithm("Load", Filename="LOQ49886.nxs", child=True)
            self.__class__._test_ws = alg.getProperty("OutputWorkspace").value

    def test_time_series_double_can_be_extracted(self):
        log_series = self._test_ws.getRun()["TEMP1"]
        self._check_has_time_series_attributes(log_series)
        self.assertEquals(log_series.size(), 63)
        self.assertAlmostEqual(log_series.nthValue(0), -0.00161)

    def test_time_series_int_can_be_extracted(self):
        log_series = self._test_ws.getRun()["raw_frames"]
        self._check_has_time_series_attributes(log_series)
        self.assertEquals(log_series.size(), 172)
        self.assertEquals(log_series.nthValue(0), 17)

    def test_time_series_string_can_be_extracted(self):
        log_series = self._test_ws.getRun()["icp_event"]
        self._check_has_time_series_attributes(log_series)
        self.assertEquals(log_series.size(), 11)
        self.assertEquals(log_series.nthValue(0).strip(), 'CHANGE_PERIOD 1')
        
        
    def _check_has_time_series_attributes(self, log):
        self.assertTrue(hasattr(log, "value"))
        self.assertTrue(hasattr(log, "times"))
        self.assertTrue(hasattr(log, "getStatistics"))

if __name__ == '__main__':
    unittest.main()
