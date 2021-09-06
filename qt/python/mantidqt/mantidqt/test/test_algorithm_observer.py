# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import AlgorithmObserver, AlgorithmManager, AlgorithmFactory, PythonAlgorithm, Progress


class MockAlgorithm(PythonAlgorithm):

    def category(self):
        return 'Tests'

    def PyInit(self):
        self.declareProperty("Error", False)

    def PyExec(self):
        error = self.getProperty("Error").value
        if error:
            raise RuntimeError('Error in algorithm')
        progress = Progress(self, 0.0, 1.0, 2)
        progress.report('Half way')
        progress.report()


AlgorithmFactory.subscribe(MockAlgorithm)


class MockObserver(AlgorithmObserver):

    def __init__(self):
        super(MockObserver, self).__init__()
        self.finish_handled = False
        self.error_message = None
        self.progress_message = None
        self.first_progress_reported = False
        self.second_progress_reported = False

    def finishHandle(self):
        self.finish_handled = True

    def errorHandle(self, message):
        self.error_message = message


class MockObserverStarting(AlgorithmObserver):

    def __init__(self):
        super(MockObserverStarting, self).__init__()
        self.starting_handled = False

    def startingHandle(self, alg):
        self.starting_handled = True


class TestAlgorithmObserver(unittest.TestCase):

    def test_starting_handle(self):
        observer = MockObserverStarting()
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
        self.assertEqual(observer.error_message, None)

    def test_error_handle(self):
        algorithm = AlgorithmManager.create("MockAlgorithm", -1)
        algorithm.setProperty("Error", True)
        observer = MockObserver()
        observer.observeFinish(algorithm)
        observer.observeError(algorithm)
        algorithm.execute()
        self.assertTrue(observer.finish_handled)
        self.assertTrue(observer.error_message.startswith('Error in algorithm'))
