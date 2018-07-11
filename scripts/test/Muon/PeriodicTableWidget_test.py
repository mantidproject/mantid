from __future__ import absolute_import, print_function

import unittest

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_widget import PeriodicTable
from Muon.GUI.Common import mock_widget

from PyQt4 import QtGui

from silx.gui.widgets import PeriodicTable as silxPT

from silx.gui.widgets.PeriodicTable import ColoredPeriodicTableItem

try:
    from unittest import mock
except ImportError:
    import mock


class MockEvent(object):
    def __init__(self):
        self.call_count = 0

    def handler(self):
        self.call_count += 1


class PeriodicTableWidgetTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.widget = PeriodicTable()
        self.view = self.widget.presenter.view
        self.mock_elem = ColoredPeriodicTableItem("T", 0, "", 1, "Test", 0)
        self.mock_symbol = "T"

        self.view.ptable = mock.create_autospec(silxPT)
        self.view.ptable.getSelection = mock.Mock(
            return_value=self.mock_symbol)
        self.view.ptable.setSelection = mock.Mock()
        self.view.ptable.isElementSelected = mock.Mock(return_value=True)
        self.view.ptable.setElementSelected = mock.Mock()

    def call_event_once(self, reg_event, func, args,
                        unregister=False, unreg_event=None):
        handler = MockEvent()
        reg_event(handler.handler)
        func(args)
        if unregister and unreg_event is not None:
            unreg_event(handler.handler)
            func(args)
        return handler.call_count

    def test_table_clicked(self):
        assert self.call_event_once(self.widget.register_table_clicked,
                                    self.view.table_clicked, self.mock_elem) == 1

    def test_unregister_table_clicked(self):
        assert self.call_event_once(self.widget.register_table_clicked,
                                    self.view.table_clicked,
                                    self.mock_elem,
                                    unregister=True,
                                    unreg_event=self.widget.unregister_table_clicked) == 1

    def test_register_table_changed(self):
        assert self.call_event_once(self.widget.register_table_changed,
                                    self.view.table_changed, [self.mock_symbol]) == 1

    def test_unregister_table_changed(self):
        assert self.call_event_once(self.widget.register_table_changed,
                                    self.view.table_changed,
                                    [self.mock_elem],
                                    unregister=True,
                                    unreg_event=self.widget.unregister_table_changed) == 1

    def test_selection(self):
        assert self.widget.selection == self.mock_symbol

    def test_is_selected(self):
        assert self.widget.is_selected(self.mock_symbol)

    def test_select_element(self):
        self.widget.select_element(self.mock_symbol, deselect=False)

    def test_add_elements(self):
        el = [self.mock_symbol] * 3
        self.widget.add_elements(*el)


if __name__ == "__main__":
    unittest.main()
