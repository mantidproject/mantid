# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from collections import Counter, namedtuple
import unittest

import qtpy
from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from mantid.api import AlgorithmFactoryImpl
from unittest.mock import Mock, patch, call
from mantidqt.utils.qt.testing import select_item_in_combo_box, select_item_in_tree, start_qapplication
from mantidqt.widgets.algorithmselector.model import AlgorithmSelectorModel
from mantidqt.widgets.algorithmselector.widget import AlgorithmSelectorWidget

AlgorithmDescriptorMock = namedtuple("AlgorithmDescriptorMock", ["name", "alias", "category", "version"])
mock_get_algorithm_descriptors = Mock()
mock_get_algorithm_descriptors.return_value = [
    AlgorithmDescriptorMock(name="Rebin", version=1, category="Transform", alias=""),
    AlgorithmDescriptorMock(name="Rebin", version=1, category="Transform\\Rebin", alias=""),
    AlgorithmDescriptorMock(name="Load", version=1, category="Data", alias=""),
    AlgorithmDescriptorMock(name="DoStuff", version=1, category="Stuff", alias=""),
    AlgorithmDescriptorMock(name="DoStuff", version=2, category="Stuff", alias=""),
    AlgorithmDescriptorMock(name="ComesFirst", version=1, category="Sorted", alias=""),
    AlgorithmDescriptorMock(name="GoesSecond", version=1, category="Sorted", alias=""),
]

empty_mock_get_algorithm_descriptors = Mock()
empty_mock_get_algorithm_descriptors.return_value = []


@patch.object(AlgorithmFactoryImpl, "getDescriptors", mock_get_algorithm_descriptors)
class ModelTest(unittest.TestCase):
    def test_get_algorithm_data(self):
        model = AlgorithmSelectorModel(None)
        names, descriptors = model.get_algorithm_data()
        self.assertTrue(isinstance(names, list))
        self.assertTrue(isinstance(descriptors, dict))
        self.assertEqual(5, len(names))
        self.assertEqual("ComesFirst", names[0])
        self.assertEqual("DoStuff", names[1])
        self.assertEqual("GoesSecond", names[2])
        self.assertEqual("Load", names[3])
        self.assertEqual("Rebin", names[4])

        self.assertTrue("Rebin" in descriptors["Transform"][AlgorithmSelectorModel.algorithm_key])
        self.assertTrue("Rebin" in descriptors["Transform"]["Rebin"][AlgorithmSelectorModel.algorithm_key])
        counter = Counter(names)
        self.assertEqual(counter["Rebin"], 1)
        self.assertEqual(counter["DoStuff"], 1)
        self.assertEqual(mock_get_algorithm_descriptors.mock_calls, [call(False, True), call(True, True)])

    def test_include_hidden_algorithms(self):
        model = AlgorithmSelectorModel(None, include_hidden=True)
        model.get_algorithm_data()
        self.assertEqual(mock_get_algorithm_descriptors.mock_calls[-1], call(True, True))


createDialogFromName_func_name = "mantidqt.interfacemanager.InterfaceManager.createDialogFromName"


@patch.object(AlgorithmFactoryImpl, "getDescriptors", mock_get_algorithm_descriptors)
@start_qapplication
class WidgetTest(unittest.TestCase):
    # def setUp(self):
    #     self.getDescriptors_orig = AlgorithmFactoryImpl.getDescriptors
    #     AlgorithmFactoryImpl.getDescriptors = mock_get_algorithm_descriptors
    #
    # def tearDown(self):
    #     AlgorithmFactoryImpl.getDescriptors = self.getDescriptors_orig

    def _select_in_tree(self, widget, item_label):
        select_item_in_tree(widget.tree, item_label)

    def _select_in_search_box(self, widget, item_text):
        select_item_in_combo_box(widget.search_box, item_text)

    def test_tree_selection_single_version(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, "Load v.1")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "Load")
        self.assertEqual(selected_algorithm.version, 1)
        self.assertEqual(widget.search_box.lineEdit().text(), "Load")

    def test_tree_selection_two_versions(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, "DoStuff v.1")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "DoStuff")
        self.assertEqual(selected_algorithm.version, 1)

        self._select_in_tree(widget, "DoStuff v.2")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "DoStuff")
        self.assertEqual(selected_algorithm.version, 2)

    def test_search_box_selection(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_search_box(widget, "Load")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "Load")
        self.assertEqual(selected_algorithm.version, -1)
        self._select_in_search_box(widget, "DoStuff")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "DoStuff")
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_filter_mode_on_qt5(self):
        if not qtpy.PYQT5:
            self.skipTest("Versions below Qt5 do not support the following functionality, and the default Qt behaviour is used")
        else:
            widget = AlgorithmSelectorWidget()
            self.assertEqual(widget.search_box.completer().filterMode(), Qt.MatchContains)

    def test_search_box_selection_ignores_tree_selection(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, "DoStuff v.2")
        self._select_in_search_box(widget, "Load")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "Load")
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_set_text(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, "DoStuff v.2")
        widget.search_box.lineEdit().setText("Load")
        selected_algorithm = widget.get_selected_algorithm()
        self.assertEqual(selected_algorithm.name, "Load")
        self.assertEqual(selected_algorithm.version, -1)

    def test_search_box_selection_wrong_text(self):
        widget = AlgorithmSelectorWidget()
        self._select_in_tree(widget, "DoStuff v.2")
        widget.search_box.lineEdit().setText("abc")
        self.assertEqual(widget.get_selected_algorithm(), None)
        self.assertEqual(widget.search_box.currentText(), "abc")

    def test_run_dialog_opens_on_execute_button_click(self):
        with patch(createDialogFromName_func_name) as createDialog:
            widget = AlgorithmSelectorWidget()
            self._select_in_tree(widget, "DoStuff v.2")
            widget.execute_button.click()
            createDialog.assert_called_once_with("DoStuff", 2, None, False, {}, "", [])

    def test_run_dialog_opens_on_return_press(self):
        with patch(createDialogFromName_func_name) as createDialog:
            widget = AlgorithmSelectorWidget()
            self._select_in_tree(widget, "DoStuff v.2")
            QTest.keyClick(widget.search_box, Qt.Key_Return)
            createDialog.assert_called_once_with("DoStuff", 2, None, False, {}, "", [])

    def test_run_dialog_opens_on_double_click(self):
        with patch(createDialogFromName_func_name) as createDialog:
            widget = AlgorithmSelectorWidget()
            self._select_in_tree(widget, "Load v.1")
            selected_item = widget.tree.selectedItems()[0]
            item_pos = widget.tree.visualItemRect(selected_item).center()
            QTest.mouseDClick(widget.tree.viewport(), Qt.LeftButton, Qt.NoModifier, pos=item_pos)
            createDialog.assert_called_once_with("Load", 1, None, False, {}, "", [])

    def test_sorting_of_algorithms(self):
        widget = AlgorithmSelectorWidget()
        model = AlgorithmSelectorModel(None)
        top_level = []

        widget._add_tree_items(top_level, model.get_algorithm_data()[1])

        self.assertEqual(top_level[0].text(0), "Data")
        self.assertEqual(top_level[1].text(0), "Sorted")
        self.assertEqual(top_level[2].text(0), "Stuff")
        self.assertEqual(top_level[3].text(0), "Transform")

        second_level = top_level[1].takeChildren()
        self.assertEqual(second_level[0].text(0), "ComesFirst v.1")
        self.assertEqual(second_level[1].text(0), "GoesSecond v.1")

    def test_refresh(self):
        # Set a mock to return an empty descriptor list
        getDescriptors_orig = AlgorithmFactoryImpl.getDescriptors
        AlgorithmFactoryImpl.getDescriptors = empty_mock_get_algorithm_descriptors

        widget = AlgorithmSelectorWidget()
        self.assertEqual(0, widget.tree.topLevelItemCount())
        # put back the original
        AlgorithmFactoryImpl.getDescriptors = getDescriptors_orig
        widget.refresh()
        self.assertEqual(4, widget.tree.topLevelItemCount())


if __name__ == "__main__":
    unittest.main()
