from __future__ import absolute_import, print_function
import unittest

from mantid.api import AlgorithmObserver, AlgorithmManager, AlgorithmFactory, PythonAlgorithm


class MockAlgorithm(PythonAlgorithm):

    def category(self):
        return 'Tests'

    def PyInit(self):
        self.declareProperty("Error", False)

    def PyExec(self):
        error = self.getPropertyValue("Error")
        if error == '1':
            raise RuntimeError('Error in algorithm')


AlgorithmFactory.subscribe(MockAlgorithm)


class MockObserver(AlgorithmObserver):

    def __init__(self):
        super(MockObserver, self).__init__()
        self.starting_handled = False
        self.finish_handled = False
        self.error_message = None

    def startingHandle(self, alg):
        self.starting_handled = True

    def finishHandle(self):
        self.finish_handled = True

    def errorHandle(self, message):
        self.error_message = message


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

    def test_finish_handle(self):
        algorithm = AlgorithmManager.create("MockAlgorithm", -1)
        observer = MockObserver()
        observer.observeFinish(algorithm)
        algorithm.execute()
        self.assertTrue(observer.finish_handled)
        self.assertTrue(observer.error_message is None)

    def test_error_handle(self):
        algorithm = AlgorithmManager.create("MockAlgorithm", -1)
        algorithm.setProperty("Error", True)
        observer = MockObserver()
        observer.observeFinish(algorithm)
        observer.observeError(algorithm)
        algorithm.execute()
        self.assertTrue(observer.finish_handled)
        self.assertTrue(observer.error_message.startswith('Error in algorithm'))
