from __future__ import absolute_import, print_function
import unittest

from mantid.api import AlgorithmObserver, AlgorithmManager, AlgorithmFactory, PythonAlgorithm
from mantid.kernel import Direction


class MockAlgorithm(PythonAlgorithm):

    def category(self):
        return 'Tests'

    def PyInit(self):
        self.declareProperty("InValue", 0)
        self.declareProperty("OutValue", 0, direction=Direction.Output)

    def PyExec(self):
        i = self.getPropertyValue("InValue")
        # print ('InValue=', i)


AlgorithmFactory.subscribe(MockAlgorithm)


class MockObserver(AlgorithmObserver):

    def __init__(self):
        super(MockObserver, self).__init__()
        self.starting_handled = False

    def startingHandle(self, alg):
        self.starting_handled = True


class TestAlgorithmObserver(unittest.TestCase):

    def test_default_observer(self):
        observer = AlgorithmObserver()
        observer.observeStarting()
        algorithm = AlgorithmManager.create("MockAlgorithm", -1)
        try:
            # Check that nothing crashes if the base class is used
            algorithm.execute()
        except:
            self.fail('Default AlgorithmObserver is broken')

    def test_starting_handle(self):
        observer = MockObserver()
        observer.observeStarting()
        algorithm = AlgorithmManager.create("MockAlgorithm", -1)
        algorithm.execute()
        self.assertTrue(observer.starting_handled)
