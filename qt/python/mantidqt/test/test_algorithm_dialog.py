import unittest

from qtpy.QtWidgets import QApplication, QWidget

from mantidqt.utils.qt.plugins import setup_library_paths
from mantid.simpleapi import AlgorithmManager
from _widgetscoreqt5 import AlgorithmDialog, GenericDialog

setup_library_paths()


class TestAlgorithmDialog(unittest.TestCase):

    def test_it_exists(self):
        self.assertTrue(issubclass(AlgorithmDialog, QWidget))
        self.assertTrue('setAlgorithm' in dir(AlgorithmDialog))
        self.assertTrue('addAlgorithmObserver' in dir(AlgorithmDialog))

    def test_generic_dialog(self):
        app = QApplication([''])
        dialog = GenericDialog()
        alg = AlgorithmManager.create('Load')
        dialog.setAlgorithm(alg)
        dialog.initializeLayout()
        dialog.show()
        app.exec_()
