from __future__ import print_function, absolute_import


import unittest
from qtpy import QtGui, QtWidgets, QtCore
from copy import deepcopy

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

import Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table as periodic_table
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTableItem,\
    ColoredPeriodicTableItem, _ElementButton, PeriodicTable, PeriodicCombo, PeriodicList


class PeriodicTableItemTest(unittest.TestCase):
    def setUp(self):
        self.element = PeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")

    def test_that_list_elements_contains_all(self):
        for i in range(len(periodic_table._elements)):
            self.assertTrue(isinstance(periodic_table._elements[i][0], str))
            self.assertTrue(isinstance(periodic_table._elements[i][1], int))
            self.assertTrue(isinstance(periodic_table._elements[i][2], int))
            self.assertTrue(isinstance(periodic_table._elements[i][3], int))
            self.assertTrue(isinstance(periodic_table._elements[i][4], str))
            self.assertTrue(isinstance(periodic_table._elements[i][5], float))
            self.assertTrue(isinstance(periodic_table._elements[i][6], str))

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


class PeriodicTableTest(GuiTest):
    def setUp(self):
        self.ptable = PeriodicTable()
        self.item = PeriodicTableItem("Ti", 22, 4, 4, "titanium", 47.9000, "transition metal")

    def test_that_silentSetElementSelected_sets_the_state_of_the_right_element(self):
        self.ptable._eltButtons['Cu'].setSelected = mock.Mock()
        self.ptable.silentSetElementSelected('Cu', 'mystate')

        self.ptable._eltButtons['Cu'].setSelected.assert_called_with('mystate')

    def test_that_enableElementButton_enables_element(self):
        self.ptable._eltButtons['Cu'].setEnabled = mock.Mock()
        self.ptable.enableElementButton('Cu')

        self.ptable._eltButtons['Cu'].setEnabled.assert_called_with(True)

    def test_that_enableElementButton_does_not_throw_with_bad_data(self):
        self.ptable.enableElementButton('this-element-does-not-exist')

    def test_that_disableElementButton_enables_element(self):
        self.ptable._eltButtons['Cu'].setEnabled = mock.Mock()
        self.ptable.disableElementButton('Cu')

        self.ptable._eltButtons['Cu'].setEnabled.assert_called_with(False)

    def test_that_disableElementButton_does_not_throw_with_bad_data(self):
        self.ptable.disableElementButton('this-element-does-not-exist')

    def test_that_isElementButtonEnabled_calls_right_function(self):
        self.ptable._eltButtons['Cu'].isEnabled = mock.Mock()
        self.ptable._eltButtons['Cu'].isEnabled.return_value = True
        res = self.ptable.isElementButtonEnabled('Cu')

        self.assertEqual(self.ptable._eltButtons['Cu'].isEnabled.call_count, 1)
        self.assertEqual(res, True)

    def test_that_isElementButtonEnabled_does_not_throw_with_bad_data(self):
        self.ptable.isElementButtonEnabled('this-element-does-not-exist')

    def test_addElement_adds_the_correct_element(self):
        self.ptable.gridLayout.addWidget = mock.Mock()
        self.ptable._addElement(self.item)

        self.assertEqual(self.ptable.gridLayout.addWidget.call_count, 1)

    def test_that_elementEnter_calls_the_right_function(self):
        self.ptable.eltLabel.setText = mock.Mock()
        self.ptable.elementEnter(self.item)

        self.ptable.eltLabel.setText.assert_called_with("Ti(22) - titanium")

    def test_that_elementLeave_clears_label(self):
        self.ptable.eltLabel.setText = mock.Mock()
        self.ptable._elementLeave(self.item)

        self.ptable.eltLabel.setText.assert_called_with('')

    def test_elementLeftClicked_calls_correct_function(self):
        self.ptable._eltButtons[self.item.symbol] = mock.Mock()
        self.ptable.sigElementLeftClicked = mock.Mock()
        self.ptable._elementLeftClicked(self.item)

        self.ptable._eltButtons[self.item.symbol].setCurrent.assert_called_with(True)
        self.assertEqual(self.ptable._eltCurrent, self.ptable._eltButtons[self.item.symbol])
        self.ptable.sigElementLeftClicked.emit.assert_called_with(self.item)

    def test_elementRightClicked_sends_correct_signal(self):
        self.ptable.sigElementRightClicked = mock.Mock()
        self.ptable._elementRightClicked(self.item)

        self.ptable.sigElementRightClicked.emit.assert_called_with(self.item)

    def test_getSelection_returns_correct_value(self):
        self.ptable._eltButtons = mock.Mock()
        el_mock = mock.Mock()
        el_mock.isSelected.return_value = True
        el_hidden_mock = mock.Mock()
        el_hidden_mock.isSelected.return_value = False
        self.ptable._eltButtons.values.return_value = [el_hidden_mock, el_mock]
        result = self.ptable.getSelection()

        self.assertEqual(result, [el_mock.item])

    def test_setSelection_sends_signal(self):
        self.ptable.sigSelectionChanged = mock.Mock()
        self.ptable.getSelection = mock.Mock()
        self.ptable.getSelection.return_value = 'test-selection'
        self.ptable.setSelection('Ti')

        self.ptable.sigSelectionChanged.emit.assert_called_with('test-selection')

    def test_that_setSelection_only_sets_for_items_in_argument(self):
        items = {'Cu': mock.Mock(), 'Ti': mock.Mock(), 'Au': mock.Mock()}
        self.ptable._eltButtons = items
        self.ptable.setSelection(['Cu', 'Au'])

        items['Cu'].setSelected.assert_called_with(True)
        items['Ti'].setSelected.assert_called_with(False)
        items['Au'].setSelected.assert_called_with(True)

    def test_setElementSelected_calls_right_functions(self):
        self.ptable._eltButtons['Cu'] = mock.Mock()
        self.ptable.sigSelectionChanged = mock.Mock()
        self.ptable.getSelection = mock.Mock()
        self.ptable.getSelection.return_value = 'my-result'
        self.ptable.setElementSelected('Cu', True)

        self.ptable._eltButtons['Cu'].setSelected.assert_called_with(True)
        self.assertEqual(self.ptable._eltButtons['Cu'].setSelected.call_count, 1)
        self.ptable.sigSelectionChanged.emit.assert_called_with('my-result')
        self.assertEqual(self.ptable.sigSelectionChanged.emit.call_count, 1)

    def test_isElementSelected_gives_correct_result(self):
        self.ptable._eltButtons['Cu'] = mock.Mock()
        self.ptable._eltButtons['Cu'].isSelected.return_value = True

        self.assertEqual(self.ptable.isElementSelected('Cu'), True)
        self.assertEqual(self.ptable._eltButtons['Cu'].isSelected.call_count, 1)

    def test_elementToggle_calls_correct_functions(self):
        self.ptable._eltButtons['Ti'] = mock.Mock()
        self.ptable._eltButtons['Ti'].isSelected.return_value = True
        self.ptable.sigSelectionChanged = mock.Mock()
        self.ptable.elementToggle(self.item)

        self.ptable._eltButtons['Ti'].setSelected.assert_called_with(False)
        self.assertEqual(self.ptable.sigSelectionChanged.emit.call_count, 1)


class PeriodicComboTest(GuiTest):
    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QComboBox.insertItem')
    def test_that_init_defaults_to_all_elements_in_periodic_table(self, mock_insert_item):
        PeriodicCombo()
        self.assertEqual(mock_insert_item.call_count, 109)
        mock_insert_item.assert_called_with(108, 'Mt (109) - meitnerium')

        PeriodicCombo(detailed=False)
        self.assertEqual(mock_insert_item.call_count, 109+109)
        mock_insert_item.assert_called_with(108, 'Mt (109)')

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QComboBox.insertItem')
    def test_that_init_accepts_new_list_elements(self, mock_insert_item):
        new_elements = [PeriodicTableItem("H", 1, 1, 1, "hydrogen", 1.00800, "diatomic nonmetal"),
                        PeriodicTableItem("He", 2, 18, 1, "helium", 4.0030, "noble gas"),
                        PeriodicTableItem("Li", 3, 1, 2, "lithium", 6.94000, "alkali metal")]
        PeriodicCombo(elements=new_elements, detailed=True)
        self.assertEqual(mock_insert_item.call_count, 3)
        mock_insert_item.assert_called_with(2, 'Li (3) - lithium')

        PeriodicCombo(elements=new_elements, detailed=False)
        self.assertEqual(mock_insert_item.call_count, 3+3)
        mock_insert_item.assert_called_with(2, 'Li (3)')

    def test_that_selection_changed_signal_sent(self):
        combo = PeriodicCombo()
        combo.sigSelectionChanged = mock.Mock()
        combo._selectionChanged(1)

        combo.sigSelectionChanged.emit.assert_called_with(periodic_table._defaultTableItems[1])

    def test_that_selected_item_is_correct(self):
        combo = PeriodicCombo()
        combo.currentIndex = mock.Mock(return_value=1)

        self.assertEqual(combo.getSelection(), periodic_table._defaultTableItems[1])

    def test_that_setSelection_sets_correct_selection_if_given_symbol(self):
        combo = PeriodicCombo()
        combo.setCurrentIndex = mock.Mock()
        combo.setSelection('H')

        combo.setCurrentIndex.assert_called_with(0)

    def test_that_setSelection_sets_correct_selection_if_given_PeriodicTableItem(self):
        combo = PeriodicCombo()
        combo.setCurrentIndex = mock.Mock()
        combo.setSelection(PeriodicTableItem("H", 1, 1, 1, "hydrogen", 1.00800, "diatomic nonmetal"))

        combo.setCurrentIndex.assert_called_with(0)

    def test_that_setSelection_throws_if_given_bad_symbol(self):
        combo = PeriodicCombo()
        with self.assertRaises(ValueError):
            combo.setSelection('I-dont-exist')


class PeriodicListTest(GuiTest):
    def setUp(self):
        self.plist = PeriodicList()

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QTreeWidgetItem')
    def test_that_fill_widget_uses_defaultTableItems_when_argument_is_none(self, mock_items):
        self.plist._fill_widget(None)

        exp_num_calls = len(periodic_table._defaultTableItems)
        self.assertEqual(mock_items.call_count, exp_num_calls)
        self.assertEqual(len(self.plist.tree_items), exp_num_calls)

    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table.QtWidgets.QTreeWidgetItem')
    def test_that_fill_widget_uses_custom_items_when_argument_is_not_none(self, mock_items):
        exp_num_calls = 3
        self.plist._fill_widget(periodic_table._defaultTableItems[:exp_num_calls])

        self.assertEqual(mock_items.call_count, exp_num_calls)
        self.assertEqual(len(self.plist.tree_items), exp_num_calls)

    def test_that_tree_items_is_filled_by_QTreeWidgetItems(self):
        self.plist._fill_widget(None)

        assert all([isinstance(item, QtWidgets.QTreeWidgetItem) for item in self.plist.tree_items])

    def test_all_tree_items_have_correct_name(self):
        self.plist._fill_widget(None)

        assert all([self.plist.tree_items[i].text(0) == str(el.Z)
                    for i, el in enumerate(periodic_table._defaultTableItems)])
        assert all([self.plist.tree_items[i].text(1) == el.symbol
                    for i, el in enumerate(periodic_table._defaultTableItems)])

    def test_that_selectionChanged_emits_correct_signal(self):
        self.plist.sigSelectionChanged = mock.Mock()
        self.plist.getSelection = mock.Mock(return_value='my-return')
        self.plist._selectionChanged(mock.Mock(), mock.Mock())

        self.plist.sigSelectionChanged.emit.assert_called_with('my-return')

    def test_that_getSelection_returns_correct_number_of_elements(self):
        item1 = mock.Mock()
        item1.isSelected.return_value = True
        item2 = mock.Mock()
        item2.isSelected.return_value = True
        item3 = mock.Mock()
        item3.isSelected.return_value = False
        item4 = mock.Mock()
        item4.isSelected.return_value = True
        self.plist.tree_items = [item1, item2, item3, item4]
        ret = self.plist.getSelection()

        self.assertEqual(len(ret), 3)
        self.assertEqual(ret, [periodic_table._defaultTableItems[0],
                               periodic_table._defaultTableItems[1],
                               periodic_table._defaultTableItems[3]])

    def test_that_setSelectedElements_throws_when_given_no_elements(self):
        with self.assertRaises(ValueError):
            self.plist.setSelectedElements([])

    def test_that_setSelectedElements_calls_setSelected_correctly_when_given_elements(self):
        item1 = mock.Mock()
        item2 = mock.Mock()
        item3 = mock.Mock()
        item4 = mock.Mock()
        self.plist.tree_items = [item1, item2, item3, item4]
        self.plist.setSelectedElements(['H', 'Li'])

        assert all([item.setSelected.call_count == 1 for item in self.plist.tree_items])
        item1.setSelected.assert_called_with(True)
        item3.setSelected.assert_called_with(True)
        item2.setSelected.assert_called_with(False)
        item4.setSelected.assert_called_with(False)


if __name__ == '__main__':
    unittest.main()























