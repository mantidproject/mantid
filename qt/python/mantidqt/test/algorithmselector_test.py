import unittest
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication, QComboBox
from mantidqt.widgets.algorithmselector.model import Model
from mantidqt.widgets.algorithmselector.widget import AlgorithmSelectorWidget
from mantidqt.utility.gui_test import meta_gui_test


class ModelTest(unittest.TestCase):

    def test_get_algorithm_data(self):
        model = Model(None)
        names, descriptors = model.get_algorithm_data()
        self.assertTrue(isinstance(names, list))
        self.assertTrue(isinstance(descriptors, dict))
        self.assertTrue('LoadAscii' in names)
        self.assertTrue('Rebin' in names)
        self.assertTrue('RealFFT' in descriptors['Arithmetic']['FFT'][Model.algorithm_key])


class WidgetTest(unittest.TestCase):

    __metaclass__ = meta_gui_test

    def _select_in_tree(self, widget, item_label):
        items = widget.tree.findItems(item_label, Qt.MatchExactly | Qt.MatchRecursive)
        widget.tree.setCurrentItem(items[0])

    def _select_in_search_box(self, widget, item_text):
        i = widget.search_box.findText(item_text)
        widget.search_box.setCurrentIndex(i)

    def test_tree_selection_single_version(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'Squares v.1')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Squares')
        self.assertEqual(selected_algorithm.version, 1)

    def test_tree_selection_two_versions(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'FindEPP v.1')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'FindEPP')
        self.assertEqual(selected_algorithm.version, 1)

        self._select_in_tree(widget, 'FindEPP v.2')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'FindEPP')
        self.assertEqual(selected_algorithm.version, 2)

    def test_search_box_selection(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_search_box(widget, 'Squares')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Squares')
        self.assertEqual(selected_algorithm.version, -1)
        self._select_in_search_box(widget, 'FindEPP')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'FindEPP')
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_ignores_tree_selection(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'FindEPP v.2')
        self._select_in_search_box(widget, 'Squares')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Squares')
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_set_text(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'FindEPP v.2')
        widget.search_box.lineEdit().setText('Squares')
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, 'Squares')
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_wrong_text(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, 'FindEPP v.2')
        widget.search_box.lineEdit().setText('abc')
        self.assertTrue(widget.get_selected_algorithm() is None)


if __name__ == '__main__':
    unittest.main()
