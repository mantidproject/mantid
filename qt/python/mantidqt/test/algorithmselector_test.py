import unittest
from PyQt4.QtCore import Qt
from collections import Counter
from mantidqt.widgets.algorithmselector.model import Model
from mantidqt.widgets.algorithmselector.widget import AlgorithmSelectorWidget
from mantidqt.utility.gui_test import gui_test_case
from mock import Mock, patch


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
        model = Model(None)
        names, descriptors = model.get_algorithm_data()
        self.assertTrue(isinstance(names, list))
        self.assertTrue(isinstance(descriptors, dict))
        self.assertTrue('Load' in names)
        self.assertTrue('Rebin' in names)
        self.assertTrue('Rebin' in descriptors['Transform'][Model.algorithm_key])
        self.assertTrue('Rebin' in descriptors['Transform']['Rebin'][Model.algorithm_key])
        counter = Counter(names)
        self.assertEqual(counter['Rebin'], 1)
        self.assertEqual(counter['DoStuff'], 1)


@gui_test_case
@patch('mantid.AlgorithmFactory.getDescriptors', mock_get_algorithm_descriptors)
class WidgetTest(unittest.TestCase):

    def _select_in_tree(self, widget, item_label):
        items = widget.tree.findItems(item_label, Qt.MatchExactly | Qt.MatchRecursive)
        widget.tree.setCurrentItem(items[0])

    def _select_in_search_box(self, widget, item_text):
        i = widget.search_box.findText(item_text)
        widget.search_box.setCurrentIndex(i)

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


if __name__ == '__main__':
    unittest.main()
