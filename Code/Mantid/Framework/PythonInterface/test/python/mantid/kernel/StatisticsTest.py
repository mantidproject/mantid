import unittest
from mantid.kernel import Stats
import math
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

    def test_getStatistics_accepts_sorted_arg(self):
        data = numpy.array([17.2,18.1,16.5,18.3,12.6])
        data = numpy.sort(data)
        stats = Stats.getStatistics(data, sorted=True)
        self.assertAlmostEqual(16.54, stats.mean, places = 10)
        self.assertAlmostEqual(2.0733, stats.standard_deviation, places = 4)
        self.assertEquals(12.6, stats.minimum)
        self.assertEquals(18.3, stats.maximum)
        self.assertEquals(17.2, stats.median)


    def test_getZScores(self):
        """Data taken from C++ test"""
        values = [12,13,9,18,7,9,14,16,10,12,7,13,14,19,10,16,12,16,19,11]
        arr = numpy.array(values,dtype=numpy.float64)

        zscore = Stats.getZscore(arr)
        self.assertAlmostEqual(1.63977, zscore[4], places = 4)
        self.assertAlmostEqual(0.32235, zscore[6], places = 4)

        modZ = Stats.getModifiedZscore(arr)
        self.assertAlmostEqual(1.23658, modZ[4], places = 4)
        self.assertAlmostEqual(0.33725, modZ[6], places = 4)

    def test_getMoments(self):
        mean = 5.
        sigma = 4.
        deltaX = .2
        numX = 200
        # calculate to have same number of points left and right of function
        offsetX = mean - (.5 * deltaX * float(numX))
        # variance about origin
        expVar = mean*mean+sigma*sigma;
        # skew about origin
        expSkew = mean*mean*mean+3.*mean*sigma*sigma;

        # x-values to try out
        indep = numpy.arange(numX, dtype=numpy.float64)
        indep = indep*deltaX + offsetX


        # y-values
        # test different type
        depend = numpy.arange(numX, dtype=numpy.int32)
        self.assertRaises(ValueError, Stats.getMomentsAboutOrigin, indep, depend)

        # now correct y values
        weightedDiff = (indep-mean)/sigma
        depend = numpy.exp(-0.5*weightedDiff*weightedDiff)/sigma/math.sqrt(2.*math.pi)

        aboutOrigin = Stats.getMomentsAboutOrigin(indep, depend)
        self.assertTrue(isinstance(aboutOrigin, numpy.ndarray))
        self.assertEquals(4, aboutOrigin.shape[0])
        self.assertAlmostEqual(1., aboutOrigin[0], places=4)
        self.assertAlmostEqual(mean, aboutOrigin[1], places=4)
        self.assertTrue(math.fabs(expVar - aboutOrigin[2]) < 0.001*expVar)
        self.assertTrue(math.fabs(expSkew - aboutOrigin[3]) < 0.001*expSkew)

        aboutMean = Stats.getMomentsAboutMean(indep, depend)
        self.assertTrue(isinstance(aboutOrigin, numpy.ndarray))
        self.assertEquals(4, aboutOrigin.shape[0])
        self.assertAlmostEqual(1., aboutMean[0], places=4)
        self.assertAlmostEqual(0., aboutMean[1], places=4)
        self.assertTrue(math.fabs(sigma*sigma - aboutMean[2]) < 0.001*expVar)
        self.assertTrue(math.fabs(0. - aboutMean[3]) < 0.0001*expSkew)

# end class definition

if __name__ == '__main__':
    unittest.main()
