import unittest
from mantid.kernel import Stats
import numpy

DELTA_PLACES = 10

class StatisticsTest(unittest.TestCase):

    def test_getStatistics_with_floats(self):
        data = numpy.array([17.2,18.1,16.5,18.3,12.6])
        stats = Stats.getStatistics(data)
        self.assertAlmostEqual(16.54, stats.mean, places = 10)
        self.assertAlmostEqual(2.0733, stats.standard_deviation, places = 4)
        self.assertEquals(12.6, stats.minimum)
        self.assertEquals(18.3, stats.maximum)
        self.assertEquals(17.2, stats.median)

        data = numpy.sort(data)
        stats = Stats.getStatistics(data, sorted=True)
        self.assertAlmostEqual(16.54, stats.mean, places = 10)
        self.assertAlmostEqual(2.0733, stats.standard_deviation, places = 4)
        self.assertEquals(12.6, stats.minimum)
        self.assertEquals(18.3, stats.maximum)
        self.assertEquals(17.2, stats.median)

    def test_getStatistics_with_ints(self):
        data = numpy.array([17,18,16,18,12])
        stats = Stats.getStatistics(data)
        self.assertAlmostEqual(16.2, stats.mean, places = 10)
        self.assertAlmostEqual(2.2271, stats.standard_deviation, places = 4)
        self.assertEquals(12, stats.minimum)
        self.assertEquals(18, stats.maximum)
        self.assertEquals(17, stats.median)

    def test_getStatistics_accepts_sorted_arg(self):
        data = numpy.array([17.2,18.1,16.5,18.3,12.6])
        data = numpy.sort(data)
        stats = Stats.getStatistics(data, sorted=True)
        self.assertAlmostEqual(16.54, stats.mean, places = 10)
        self.assertAlmostEqual(2.0733, stats.standard_deviation, places = 4)
        self.assertEquals(12.6, stats.minimum)
        self.assertEquals(18.3, stats.maximum)
        self.assertEquals(17.2, stats.median)

if __name__ == '__main__':
    unittest.main()
