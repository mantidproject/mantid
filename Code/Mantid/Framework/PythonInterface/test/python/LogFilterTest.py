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
        self.assertEquals(height_log.size(), 26)
        period_log = run_info.getLogData("period 7")
        self.assertTrue(isinstance(period_log, BoolTimeSeriesProperty))

        filter = LogFilter(height_log)
        self.assertTrue(isinstance(filter, LogFilter))
        filter.addFilter(period_log)
        filtered_log = filter.data()
        self.assertEquals(filtered_log.size(), 6)
        
        self.assertAlmostEqual(filtered_log.nthValue(0), -1.27995)
        self.assertEqual(str(filtered_log.nthTime(0)), "2008-06-17T11:10:44")
        self.assertAlmostEqual(filtered_log.nthValue(5), 0.0)
        self.assertEqual(str(filtered_log.nthTime(5)), "2008-06-17T11:20:17")

if __name__ == '__main__':
    unittest.main()
