#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
import unittest

from qtpy.QtWidgets import QWidget, QLineEdit

from mantid.api import AlgorithmManager, AlgorithmFactory, PythonAlgorithm
from mantid.kernel import Direction, FloatArrayProperty
from mantidqt.utils.qt.testing import requires_qapp
from mantidqt.widgets.algorithmdialog import AlgorithmDialog
from mantidqt.widgets.genericdialog import GenericDialog
from mantidqt.widgets.interfacemanager import InterfaceManager


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


@requires_qapp
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


if __name__ == '__main__':
    unittest.main()
