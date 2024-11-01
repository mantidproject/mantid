# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy
from mantid.simpleapi import AddSampleLog, CreateWorkspace, DeleteWorkspace, FilterLogByTime, LoadLog


class FilterLogByTimeTest(unittest.TestCase):

    __ws = None

    """ Log file contents.
    2008-06-17T11:10:44    -0.86526
    2008-06-17T11:10:45    -1.17843
    2008-06-17T11:10:47    -1.27995
    2008-06-17T11:20:15    -1.38216
    2008-06-17T11:20:16    -1.87435
    2008-06-17T11:20:17    -2.70547
    2008-06-17T11:20:19    -2.99125
    2008-06-17T11:20:20    -3
    2008-06-17T11:20:27    -2.98519
    2008-06-17T11:20:29    -2.68904
    2008-06-17T11:20:30    -2.5
    2008-06-17T11:20:38    -2.45909
    2008-06-17T11:20:39    -2.08764
    2008-06-17T11:20:40    -2
    2008-06-17T11:20:50    -1.85174
    2008-06-17T11:20:51    -1.51258
    2008-06-17T11:20:52    -1.5
    2008-06-17T11:21:01    -1.48566
    2008-06-17T11:21:02    -1.18799
    2008-06-17T11:21:04    -1
    2008-06-17T11:21:11    -0.98799
    2008-06-17T11:21:13    -0.63694
    2008-06-17T11:21:14    -0.5
    2008-06-17T11:21:23    -0.46247
    2008-06-17T11:21:24    -0.08519
    2008-06-17T11:21:25    0
    2008-06-17T11:21:32    0
    """

    def setUp(self):
        x = numpy.arange(0, 1, 0.25)
        ws = CreateWorkspace(UnitX="1/q", DataX=x, DataY=[0, 0, 0], NSpec=1)
        self.__ws = ws
        LoadLog(Workspace=self.__ws, Filename="CSP78173_height.txt", Names="height")

    def tearDown(self):
        DeleteWorkspace(self.__ws)

    def test_startdate_after_enddate(self):
        try:
            FilterLogByTime(InputWorkspace=self.__ws, LogName="height", StartTime=1, EndTime=0)
            self.fail("End time < Start time.")
        except RuntimeError:
            pass

    def test_without_limits(self):
        AddSampleLog(Workspace=self.__ws, LogName="run_start", LogText="1900-Jan-01 00:00:00")
        AddSampleLog(Workspace=self.__ws, LogName="run_end", LogText="2100-Jan-02 00:00:00")

        results, stats = FilterLogByTime(InputWorkspace=self.__ws, LogName="height")
        self.assertTrue(isinstance(results, numpy.ndarray), "Should give back an array")
        self.assertTrue(isinstance(stats, float), "Should give back a single result")
        expected_size = self.__ws.getRun().getLogData("height").size()
        actual_size = results.size
        self.assertEqual(expected_size, actual_size, "Nothing filtered out")

    def test_with_start_limit(self):
        AddSampleLog(Workspace=self.__ws, LogName="run_start", LogText="2008-06-17T11:10:44")
        AddSampleLog(Workspace=self.__ws, LogName="run_end", LogText="2100-Jan-02 00:00:00")

        results, stats = FilterLogByTime(InputWorkspace=self.__ws, LogName="height", StartTime=1)
        self.assertTrue(isinstance(results, numpy.ndarray), "Should give back an array")
        self.assertTrue(isinstance(stats, float), "Should give back a single result")
        expected_size = self.__ws.getRun().getLogData("height").size() - 1
        actual_size = results.size
        self.assertEqual(
            expected_size, actual_size, "Should filter one out expected_size %s, actual_size %s" % (str(expected_size), str(actual_size))
        )

    def test_with_end_limit(self):
        AddSampleLog(Workspace=self.__ws, LogName="run_start", LogText="2008-06-17T11:10:44")
        AddSampleLog(Workspace=self.__ws, LogName="run_end", LogText="2100-Jan-02 00:00:00")

        results, stats = FilterLogByTime(InputWorkspace=self.__ws, LogName="height", EndTime=0.99)
        self.assertTrue(isinstance(results, numpy.ndarray), "Should give back an array")
        self.assertTrue(isinstance(stats, float), "Should give back a single result")
        expected_size = 1
        actual_size = results.size
        self.assertEqual(expected_size, actual_size, "Expected_size %s, actual_size %s" % (str(expected_size), str(actual_size)))

    def test_with_both_limits(self):
        AddSampleLog(Workspace=self.__ws, LogName="run_start", LogText="2008-06-17T11:10:44")
        AddSampleLog(Workspace=self.__ws, LogName="run_end", LogText="2100-Jan-02 00:00:00")

        results, stats = FilterLogByTime(InputWorkspace=self.__ws, LogName="height", StartTime=1, EndTime=3)
        self.assertTrue(isinstance(results, numpy.ndarray), "Should give back an array")
        self.assertTrue(isinstance(stats, float), "Should give back a single result")
        expected_size = 2
        actual_size = results.size
        self.assertEqual(
            expected_size, actual_size, "Should filter one out expected_size %s, actual_size %s" % (str(expected_size), str(actual_size))
        )
        self.assertEqual(stats, (-1.17843 - 1.27995) / 2, "The 2nd and 3rd entry. Default stats should be mean of these.")

    def __doStats(self, method):
        AddSampleLog(Workspace=self.__ws, LogName="run_start", LogText="2008-06-17T11:10:44")
        AddSampleLog(Workspace=self.__ws, LogName="run_end", LogText="2100-Jan-02 00:00:00")

        results, stats = FilterLogByTime(InputWorkspace=self.__ws, LogName="height", StartTime=1, EndTime=3, Method=method)
        self.assertTrue(isinstance(results, numpy.ndarray), "Should give back an array")
        self.assertTrue(isinstance(stats, float), "Should give back a single result")
        return stats

    def test_calculate_mean(self):
        stats = self.__doStats("mean")
        self.assertEqual(stats, (-1.17843 - 1.27995) / 2)

    def test_calculate_max(self):
        stats = self.__doStats("max")
        self.assertEqual(stats, -1.17843)

    def test_calculate_min(self):
        stats = self.__doStats("min")
        self.assertEqual(stats, -1.27995)


if __name__ == "__main__":
    unittest.main()
