from __future__ import print_function, absolute_import


import unittest
from qtpy import QtGui, QtWidgets, QtCore
from copy import deepcopy

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

import Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table as periodic_table
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTableItem,\
    ColoredPeriodicTableItem, _ElementButton


class PeriodicTableItemTest(unittest.TestCase):
    def setUp(self):
        self.element = PeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")

    def test_that_list_elements_contains_all(self):
        self.assertEqual(len(periodic_table._elements), 109)

    def test_that_get_method_works(self):
        expected = ["Ti", 22, 4, 4, "titanium", 47.9000]
        returned = [self.element[i] for i in range(6)]

        self.assertEqual(expected, returned)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table._logger.warning')
    def test_that_density_returns_a_warning(self, mock_warning):
        self.assertEqual(self.element[6], 0.0)
        self.assertEqual(mock_warning.call_count, 1)

    def test_that_length_gives_correct_value(self):
        self.assertEqual(self.element.__len__(), 6)


class ColoredPeriodicTableItemTest(unittest.TestCase):
    def setUp(self):
        self.coloured = ColoredPeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")
        self.custom = ColoredPeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal", '#ABCDEF')

    def test_that_automatic_color_is_initialized(self):
        self.assertEqual(self.coloured.bgcolor, "#FFA07A")

    def test_that_custom_colors_are_allowed(self):
        self.assertEqual(self.custom.bgcolor, '#ABCDEF')


class _ElementButtonTest(GuiTest):
    def setUp(self):
        self.item = ColoredPeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")
        self.element = _ElementButton(self.item)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table._ElementButton._setBrush')
    def test_that_set_current_updates_value_and_calls_setBrush(self, mock_setBrush):
        self.element.setCurrent(None)

        self.assertEqual(self.element.current, None)
        self.assertEqual(mock_setBrush.call_count, 1)

    def test_that_is_current_returns_correct_value(self):
        self.assertEqual(self.element.current, self.element.isCurrent())

    def test_that_is_selected_returns_correct_value(self):
        self.assertEqual(self.element.selected, self.element.isSelected())

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table._ElementButton._setBrush')
    def test_that_set_selected_updates_value_and_calls_setBrush(self, mock_setBrush):
        self.element.setSelected(None)

        self.assertEqual(self.element.selected, None)
        self.assertEqual(mock_setBrush.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtGui.QBrush')
    def test_that_brush_is_called_if_element_selected(self, mock_brush):
        self.element.selected = True
        self.element.selected_color = 'mycolor'
        self.element.palette = mock.Mock()
        self.element.setPalette = mock.Mock()
        self.element.update = mock.Mock()
        self.element._setBrush()

        self.assertEqual(mock_brush.call_count, 1)
        mock_brush.assert_called_with('mycolor')
        self.assertEqual(self.element.setPalette.call_count, 1)
        self.assertEqual(self.element.update.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtGui.QBrush')
    def test_that_brush_is_called_if_element_not_selected_and_bgcolor_not_none(self, mock_brush):
        self.element.selected = False
        self.element.bgcolor = 'mybgcolor'
        self.element.palette = mock.Mock()
        self.element.setPalette = mock.Mock()
        self.element.update = mock.Mock()
        self.element._setBrush()

        self.assertEqual(mock_brush.call_count, 1)
        mock_brush.assert_called_with('mybgcolor')
        self.assertEqual(self.element.setPalette.call_count, 1)
        self.assertEqual(self.element.update.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtGui.QBrush')
    def test_that_brush_is_called_if_element_not_selected_and_bgcolor_none(self, mock_brush):
        self.element.selected = False
        self.element.bgcolor = None
        self.element.palette = mock.Mock()
        self.element.setPalette = mock.Mock()
        self.element.update = mock.Mock()
        self.element._setBrush()

        self.assertEqual(mock_brush.call_count, 1)
        mock_brush.assert_called_with()
        self.assertEqual(self.element.setPalette.call_count, 1)
        self.assertEqual(self.element.update.call_count, 1)


if __name__ == '__main__':
    unittest.main()























