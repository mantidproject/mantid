import unittest

from qtpy.QtWidgets import QWidget, QLineEdit

from mantid.api import PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty
from mantid.simpleapi import AlgorithmManager, AlgorithmFactory
from mantidqt.utils.qt.plugins import setup_library_paths
from mantidqt.utils.qt.testing import gui_test_case

from _widgetscoreqt5 import AlgorithmDialog, GenericDialog, InterfaceManager


setup_library_paths()


class AlgorithmDialogMockAlgorithm(PythonAlgorithm):

    def category(self):
        return 'Examples'

    def PyInit(self):
        self.declareProperty("InValue", 0)
        self.declareProperty("DoubleValue", 1.0)
        self.declareProperty(FloatArrayProperty("Floats", direction=Direction.Input))
        self.declareProperty("OutValue", 0, direction=Direction.Output)

    def PyExec(self):
        pass


@gui_test_case
class TestAlgorithmDialog(unittest.TestCase):

    def setUp(self):
        AlgorithmFactory.subscribe(AlgorithmDialogMockAlgorithm)

    def tearDown(self):
        AlgorithmFactory.unsubscribe('AlgorithmDialogMockAlgorithm', 1)

    def test_it_exists(self):
        self.assertTrue(issubclass(AlgorithmDialog, QWidget))
        self.assertTrue('setAlgorithm' in dir(AlgorithmDialog))
        self.assertTrue('addAlgorithmObserver' in dir(AlgorithmDialog))

    def test_generic_dialog(self):
        dialog = GenericDialog()
        alg = AlgorithmManager.create('AlgorithmDialogMockAlgorithm')
        dialog.setAlgorithm(alg)
        dialog.initializeLayout()
        input_widgets = dialog.findChildren(QLineEdit)
        self.assertEqual(len(input_widgets), 3)

    def test_interface_manager(self):
        manager = InterfaceManager()
        dialog = manager.createDialogFromName("AlgorithmDialogMockAlgorithm", -1)
        self.assertTrue(dialog is not None)
        input_widgets = dialog.findChildren(QLineEdit)
        self.assertEqual(len(input_widgets), 3)
