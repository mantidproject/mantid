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
from __future__ import absolute_import

from collections import Counter, namedtuple
from mock import Mock, patch, call
import unittest

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from mantidqt.utils.qt.test import select_item_in_combo_box, select_item_in_tree, GuiTest
from mantidqt.widgets.algorithmselector.model import AlgorithmSelectorModel
from mantidqt.widgets.algorithmselector.widget import AlgorithmSelectorWidget


AlgorithmDescriptorMock = namedtuple('AlgorithmDescriptorMock', ['name', 'alias', 'category', 'version'])
mock_get_algorithm_descriptors = Mock()
mock_get_algorithm_descriptors.return_value = [AlgorithmDescriptorMock(name='Rebin', version=1,
                                                                       category='Transform', alias=''),
                                               AlgorithmDescriptorMock(name='Rebin', version=1,
                                                                       category='Transform\\Rebin', alias=''),
                                               AlgorithmDescriptorMock(name='Load', version=1,
                                                                       category='Data', alias=''),
                                               AlgorithmDescriptorMock(name='DoStuff', version=1,
                                                                       category='Stuff', alias=''),
                                               AlgorithmDescriptorMock(name='DoStuff', version=2,
                                                                       category='Stuff', alias=''),
                                               ]


class AlgorithmFactoryTest(unittest.TestCase):

    def test_getDescriptors(self):
        from mantid import AlgorithmFactory
        descriptors = AlgorithmFactory.getDescriptors(True)
        self.assertGreater(len(descriptors), 0)
        d = descriptors[0]
        self.assertFalse(isinstance(d, AlgorithmDescriptorMock))
        self.assertTrue(hasattr(d, 'name'))
        self.assertTrue(hasattr(d, 'alias'))
        self.assertTrue(hasattr(d, 'category'))
        self.assertTrue(hasattr(d, 'version'))


@patch('mantid.AlgorithmFactory.getDescriptors', mock_get_algorithm_descriptors)
class ModelTest(unittest.TestCase):

    def test_get_algorithm_data(self):
        model = AlgorithmSelectorModel(None)
        names, descriptors = model.get_algorithm_data()
        self.assertTrue(isinstance(names, list))
        self.assertTrue(isinstance(descriptors, dict))
        self.assertTrue('Load' in names)
        self.assertTrue('Rebin' in names)
        self.assertTrue('Rebin' in descriptors['Transform'][AlgorithmSelectorModel.algorithm_key])
        self.assertTrue('Rebin' in descriptors['Transform']['Rebin'][AlgorithmSelectorModel.algorithm_key])
        counter = Counter(names)
        self.assertEqual(counter['Rebin'], 1)
        self.assertEqual(counter['DoStuff'], 1)
        self.assertEqual(mock_get_algorithm_descriptors.mock_calls[-1], call(False))

    def test_include_hidden_algorithms(self):
        model = AlgorithmSelectorModel(None, include_hidden=True)
        model.get_algorithm_data()
        self.assertEqual(mock_get_algorithm_descriptors.mock_calls[-1], call(True))


@patch('mantid.AlgorithmFactory.getDescriptors', mock_get_algorithm_descriptors)
class WidgetTest(GuiTest):

    def _select_in_tree(self, widget, item_label):
        select_item_in_tree(widget.tree, item_label)

    def _select_in_search_box(self, widget, item_text):
        select_item_in_combo_box(widget.search_box, item_text)

    def test_tree_selection_single_version(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'Load v.1')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Load')
        self.assertEqual(selected_algorithm.version, 1)
        self.assertEqual(widget.search_box.lineEdit().text(), 'Load')

    def test_tree_selection_two_versions(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'DoStuff v.1')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'DoStuff')
        self.assertEqual(selected_algorithm.version, 1)

        self._select_in_tree(widget, 'DoStuff v.2')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'DoStuff')
        self.assertEqual(selected_algorithm.version, 2)

    def test_search_box_selection(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_search_box(widget, 'Load')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Load')
        self.assertEqual(selected_algorithm.version, -1)
        self._select_in_search_box(widget, 'DoStuff')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'DoStuff')
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_ignores_tree_selection(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'DoStuff v.2')
        self._select_in_search_box(widget, 'Load')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Load')
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_set_text(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'DoStuff v.2')
        widget.search_box.lineEdit().setText('Load')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Load')
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_wrong_text(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'DoStuff v.2')
        widget.search_box.lineEdit().setText('abc')
        self.assertTrue(widget.get_selected_algorithm() is None)
        self.assertEqual(widget.search_box.currentText(), 'abc')

    def test_execute_on_click(self):
        with patch('mantidqt.interfacemanager.InterfaceManager.createDialogFromName') as createDialog:
            widget = AlgorithmSelectorWidget()
            self._select_in_tree(widget, 'DoStuff v.2')
            widget.execute_button.click()
            createDialog.assert_called_once_with('DoStuff', 2)

    def test_execute_on_return_press(self):
        with patch('mantidqt.interfacemanager.InterfaceManager.createDialogFromName') as createDialog:
            widget = AlgorithmSelectorWidget()
            self._select_in_tree(widget, 'DoStuff v.2')
            QTest.keyClick(widget.search_box, Qt.Key_Return)
            createDialog.assert_called_once_with('DoStuff', 2)


if __name__ == '__main__':
    unittest.main()
