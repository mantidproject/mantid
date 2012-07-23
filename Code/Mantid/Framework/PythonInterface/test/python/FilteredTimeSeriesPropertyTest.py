import unittest
from mantid.kernel import FloatFilteredTimeSeriesProperty
import testhelpers

class FilteredTimeSeriesPropertyTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self.__class__._test_ws is None:
            alg = testhelpers.run_algorithm("LoadRaw", Filename="CSP78173.raw", OutputWorkspace='test', child=True)
            self.__class__._test_ws = alg.getProperty("OutputWorkspace_7").value

    def test_filtered_property_gives_back_correct_unfiltered_one(self):
        run_info = self._test_ws.getRun()
        height_log = run_info.getLogData("height")
        
        self.assertTrue(isinstance(height_log, FloatFilteredTimeSeriesProperty))
        self.assertEquals(height_log.size(), 6)
        
        unfiltered_height = height_log.unfiltered()
        self.assertEquals(unfiltered_height.size(), 6)
        
if __name__ == '__main__':
    unittest.main()
