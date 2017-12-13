import unittest

from qtpy.QtWidgets import QWidget

from _widgetscoreqt5 import AlgorithmDialog


class TestAlgorithmDialog(unittest.TestCase):

    def test_it_exists(self):
        self.assertTrue(issubclass(AlgorithmDialog, QWidget))
        self.assertTrue('setAlgorithm' in dir(AlgorithmDialog))
        self.assertTrue('addAlgorithmObserver' in dir(AlgorithmDialog))
