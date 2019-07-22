from __future__ import print_function, absolute_import


import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTableItem,\
    ColoredPeriodicTableItem, _ElementButton, PeriodicTable, PeriodicCombo, PeriodicList

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

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QPushButton.paintEvent')
    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtGui.QPainter.fillRect')
    def test_that_paint_event_does_not_call_fillRect_is_brush_is_none(self, mock_fillRect, mock_paintEvent):
        self.element.brush = None
        self.element.paintEvent(mock.Mock())

        self.assertEqual(mock_fillRect.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QPushButton.paintEvent')
    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtGui.QPainter.fillRect')
    def test_that_paint_event_calls_fillRect_is_brush_is_not_none(self, mock_fillRect, mock_paintEvent):
        self.element.brush = mock.Mock()
        self.element.paintEvent(mock.Mock())

        self.assertEqual(mock_fillRect.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QPushButton.paintEvent')
    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtGui.QPen.setWidth')
    def test_that_setWidth_called_with_correct_parameters(self, mock_setWidth, mock_paintEvent):
        self.element.isCurrent = mock.Mock()
        self.element.isCurrent.return_value = True
        self.element.paintEvent(mock.Mock())
        mock_setWidth.assert_called_with(5)

        self.element.isCurrent.return_value = False
        self.element.paintEvent(mock.Mock())
        mock_setWidth.assert_called_with(1)

    def test_that_enterEvent_calls_the_correct_function(self):
        self.element.sigElementEnter = mock.Mock()
        self.element.enterEvent(mock.Mock())

        self.element.sigElementEnter.emit.assert_called_with(self.element.item)
        self.assertEqual(self.element.sigElementEnter.emit.call_count, 1)

    def test_that_leaveEvent_calls_the_correct_function(self):
        self.element.sigElementLeave = mock.Mock()
        self.element.leaveEvent(mock.Mock())

        self.element.sigElementLeave.emit.assert_called_with(self.element.item)
        self.assertEqual(self.element.sigElementLeave.emit.call_count, 1)

    def test_that_leftClickedSlot_calls_the_correct_function(self):
        self.element.sigElementLeftClicked = mock.Mock()
        self.element.leftClickedSlot()

        self.element.sigElementLeftClicked.emit.assert_called_with(self.element.item)
        self.assertEqual(self.element.sigElementLeftClicked.emit.call_count, 1)

    def test_that_rightClickedSlot_calls_the_correct_function(self):
        self.element.sigElementRightClicked = mock.Mock()
        self.element.rightClickedSlot()

        self.element.sigElementRightClicked.emit.assert_called_with(self.element.item)
        self.assertEqual(self.element.sigElementRightClicked.emit.call_count, 1)


if __name__ == '__main__':
    unittest.main()
