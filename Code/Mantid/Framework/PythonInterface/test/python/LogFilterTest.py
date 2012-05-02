import unittest
import testhelpers
from mantid.kernel import LogFilter, FloatTimeSeriesProperty, BoolTimeSeriesProperty


class LogFilterTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self.__class__._test_ws is None:
            alg = testhelpers.run_algorithm("LoadRaw", Filename="CSP78173.raw", OutputWorkspace='test', child=True)
            self.__class__._test_ws = alg.getProperty("OutputWorkspace_7").value

    def test_filter_by_period(self):
        run_info = self._test_ws.getRun()
        height_log = run_info.getLogData("height")
        self.assertEquals(len(height_log.value), 26)
        period_log = run_info.getLogData("period 7")
        self.assertTrue(isinstance(period_log, BoolTimeSeriesProperty))

        filter = LogFilter(height_log)
        self.assertTrue(isinstance(filter, LogFilter))
        

if __name__ == '__main__':
    unittest.main()
