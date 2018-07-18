from __future__ import absolute_import, print_function

import unittest

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTable
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTable as silxPT

from Muon.GUI.Common import mock_widget


try:
    from unittest import mock
except ImportError:
    import mock


class PeriodicTableWidgetTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.widget = PeriodicTable()
        self.view = self.widget.view
        self.mock_elem = mock.Mock()

        self.view.ptable = mock.create_autospec(silxPT)
        self.view.ptable.getSelection = mock.Mock(
            return_value=self.mock_elem)
        self.view.ptable.setSelection = mock.Mock()
        self.view.ptable.isElementSelected = mock.Mock(return_value=True)
        self.view.ptable.setElementSelected = mock.Mock()

    def emit_once(self, reg_func, slot,
                  unregister=False, unreg_func=None):
        handler = mock.Mock()
        reg_func(handler)
        slot(mock.Mock())
        if unregister and unreg_func is not None:
            unreg_func(handler)
            slot(mock.Mock())
        assert handler.call_count == 1

    def call_func_once(self, func):
        self.widget.view = mock.create_autospec(PeriodicTableView)
        func = mock.Mock()
        func(mock.Mock())
        assert func.call_count == 1

    def test_register_table_lclicked(self):
        self.call_func_once(self.widget.register_table_lclicked)

    def test_unregister_table_lclicked(self):
        self.call_func_once(self.widget.unregister_table_lclicked)

    def test_register_table_rclicked(self):
        self.call_func_once(self.widget.register_table_rclicked)

    def test_unregister_table_rclicked(self):
        self.call_func_once(self.widget.unregister_table_rclicked)

    def test_register_lclicked_emit(self):
        self.emit_once(self.widget.register_table_lclicked,
                       self.view.table_left_clicked)

    def test_unregister_lclicked_emit(self):
        self.emit_once(self.widget.register_table_lclicked,
                       self.view.table_left_clicked,
                       unregister=True,
                       unreg_func=self.widget.unregister_table_lclicked)

    def test_register_rclicked_emit(self):
        self.emit_once(self.widget.register_table_rclicked,
                       self.view.table_right_clicked)

    def test_unregister_rclicked_emit(self):
        self.emit_once(self.widget.register_table_rclicked,
                       self.view.table_right_clicked,
                       unregister=True,
                       unreg_func=self.widget.unregister_table_rclicked)

    def test_register_table_changed(self):
        self.emit_once(self.widget.register_table_changed,
                       self.view.table_changed)

    def test_unregister_table_changed(self):
        self.emit_once(self.widget.register_table_changed,
                       self.view.table_changed,
                       unregister=True,
                       unreg_func=self.widget.unregister_table_changed)

    def test_selection(self):
        assert self.widget.selection == self.mock_elem

    def test_is_selected(self):
        self.call_func_once(self.widget.is_selected)

    def test_select_element(self):
        self.call_func_once(self.widget.select_element)

    def test_add_elements(self):
        self.call_func_once(self.widget.add_elements)


if __name__ == "__main__":
    unittest.main()
