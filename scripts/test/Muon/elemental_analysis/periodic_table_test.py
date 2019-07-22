from __future__ import print_function, absolute_import

import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTableItem, PeriodicTable


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


if __name__ == '__main__':
    unittest.main()
