# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import unittest

from qtpy.QtWidgets import QWidget, QLineEdit

from mantid.api import AlgorithmManager, AlgorithmFactory, PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty
from mantidqt.dialogs.algorithmdialog import AlgorithmDialog
from mantidqt.dialogs.genericdialog import GenericDialog
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt.testing import GuiTest


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


class TestAlgorithmDialog(GuiTest):

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
        self.assertNotEqual(dialog, None)
        input_widgets = dialog.findChildren(QLineEdit)
        self.assertEqual(len(input_widgets), 3)


if __name__ == '__main__':
    unittest.main()
