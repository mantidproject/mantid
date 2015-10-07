import unittest
import numpy

from mantid.kernel import *
from mantid.api import *
from mantid import config
from testhelpers import run_algorithm

class SANSSubtractTest(unittest.TestCase):

    def setUp(self):
        config['default.instrument'] = 'EQSANS'
        config["default.facility"] = "SNS"
        x = numpy.asarray([0,1,2,3,4,5])
        y = numpy.asarray([1,2,3,4,5])

        alg = run_algorithm('CreateWorkspace',
                            DataX = x,
                            DataY = y,
                            OutputWorkspace='_test_iq_ws'
                            )
        self.test_ws = alg.getPropertyValue("OutputWorkspace")

    def test_simple_subtraction(self):
        run_algorithm(
            'SANSSubtract',
            DataDistribution=self.test_ws,
            Background=self.test_ws,
            OutputWorkspace = 'test',
            rethrow = True)
        self.assertTrue(AnalysisDataService.doesExist('test'))

        y = AnalysisDataService.retrieve('test').readY(0)
        for value in y:
            self.assertAlmostEqual(value, 0, 2)

if __name__ == '__main__':
    unittest.main()