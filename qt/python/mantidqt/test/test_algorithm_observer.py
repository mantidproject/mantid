# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function
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

    def progressHandle(self, p, message):
        if len(message) > 0:
            self.progress_message = message
        if p == 0.5:
            self.first_progress_reported = True
        if p == 1.0:
            self.second_progress_reported = True


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

    def test_progress_handle(self):
        algorithm = AlgorithmManager.create("MockAlgorithm", -1)
        observer = MockObserver()
        observer.observeProgress(algorithm)
        algorithm.execute()
        self.assertTrue(observer.first_progress_reported)
        self.assertTrue(observer.second_progress_reported)
        self.assertEqual(observer.progress_message, 'Half way')
