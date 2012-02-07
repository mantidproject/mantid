import unittest
from testhelpers import run_algorithm

class TimeSeriesPropertyTest(unittest.TestCase):

    def test_time_series_can_be_extracted(self):
        alg = run_algorithm("Load", Filename="CNCS_7860_event.nxs", child=True)
        ws = alg.getProperty("OutputWorkspace").value
        log_series = ws.getRun()["Phase1"]
        self.assertTrue(hasattr(log_series, "value"))
        self.assertTrue(hasattr(log_series, "times"))
        self.assertTrue(hasattr(log_series, "getStatistics"))

if __name__ == '__main__':
    unittest.main()
