from __future__ import print_function
from collections import Counter
from mock import Mock, patch, call
import unittest
from PyQt4.QtTest import QTest
from mantidqt.widgets.algorithmselector.model import AlgorithmSelectorModel
from mantidqt.widgets.algorithmselector.widget import AlgorithmSelectorWidget
from mantidqt.utils.qt.testing import *


mock_get_algorithm_descriptors = Mock()
mock_get_algorithm_descriptors.return_value = [['Rebin', 1, 'Transform', ''],
                                               ['Rebin', 1, 'Transform\\Rebin', ''],
                                               ['Load', 1, 'Data', ''],
                                               ['DoStuff', 1, 'Stuff', ''],
                                               ['DoStuff', 2, 'Stuff', ''],
                                               ]


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


@gui_test_case
@patch('mantid.AlgorithmFactory.getDescriptors', mock_get_algorithm_descriptors)
class WidgetTest(unittest.TestCase):

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

    def test_execute_signal(self):
        slot = Mock()
        widget = AlgorithmSelectorWidget()
        widget.execute_selected_algorithm.connect(slot)
        self._select_in_tree(widget, 'DoStuff v.2')
        widget.execute_button.click()
        slot.assert_called_once_with('DoStuff', 2)

    def test_execute_return_press(self):
        slot = Mock()
        widget = AlgorithmSelectorWidget()
        widget.execute_selected_algorithm.connect(slot)
        self._select_in_tree(widget, 'DoStuff v.2')
        QTest.keyClick(widget.search_box, Qt.Key_Return)
        slot.assert_called_once_with('DoStuff', 2)


if __name__ == '__main__':
    unittest.main()
