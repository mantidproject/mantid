from __future__ import absolute_import, print_function

import unittest

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTable
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTable as silxPT

from Muon.GUI.Common import mock_widget


try:
    from unittest import mock
except ImportError:
    import mock


class PeriodicTableWidgetTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self._model = mock.create_autospec(PeriodicTableModel)
        self.widget = PeriodicTable(PeriodicTableView(), self._model)
        self.mock_elem = mock.Mock()

        self.widget.view.ptable = mock.create_autospec(silxPT)
        self.widget.view.ptable.getSelection = mock.Mock(
            return_value=self.mock_elem)
        self.widget.view.ptable.isElementSelected = mock.Mock(
            return_value=True)
        self.widget.view.ptable.setSelection = mock.Mock()
        self.widget.view.ptable.setElementSelected = mock.Mock()

        self.widget.is_selected = mock.Mock()

        self.widget.view.on_table_lclicked = mock.Mock()
        self.widget.view.on_table_rclicked = mock.Mock()
        self.widget.view.on_table_changed = mock.Mock()

        self.widget.view.unreg_on_table_lclicked = mock.Mock()
        self.widget.view.unreg_on_table_rclicked = mock.Mock()
        self.widget.view.unreg_on_table_changed = mock.Mock()

        self.view = self.widget.view

    # checks if subsequent function is called on func()
    def check_second_func_called(self, func, sub_func):
        func(mock.Mock())
        assert sub_func.call_count == 1

    def test_register_table_lclicked(self):
        self.check_second_func_called(
            self.widget.register_table_lclicked,
            self.view.on_table_lclicked)

    def test_unregister_table_lclicked(self):
        self.check_second_func_called(
            self.widget.unregister_table_lclicked,
            self.view.unreg_on_table_lclicked)

    def test_register_table_rclicked(self):
        self.check_second_func_called(
            self.widget.register_table_rclicked,
            self.view.on_table_rclicked)

    def test_unregister_table_rclicked(self):
        self.check_second_func_called(
            self.widget.unregister_table_rclicked,
            self.view.unreg_on_table_rclicked)

    def test_register_table_changed(self):
        self.check_second_func_called(
            self.widget.register_table_changed,
            self.view.on_table_changed)

    def test_unregister_table_changed(self):
        self.check_second_func_called(
            self.widget.unregister_table_changed,
            self.view.unreg_on_table_changed)

    def test_selection(self):
        assert self.widget.selection == self.mock_elem

    def test_is_selected(self):
        assert self.widget.is_selected(mock.Mock())

    def test_select_element(self):
        self.check_second_func_called(
            self.widget.select_element,
            self.view.ptable.setElementSelected)

    def test_add_elements(self):
        self.check_second_func_called(
            self.widget.add_elements,
            self.view.ptable.setSelection)


if __name__ == "__main__":
    unittest.main()
