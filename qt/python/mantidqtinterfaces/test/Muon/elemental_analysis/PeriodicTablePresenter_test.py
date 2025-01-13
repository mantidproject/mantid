# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import PeriodicTable as silxPT, PeriodicTableItem
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView


@start_qapplication
class PeriodicTablePresenterTest(unittest.TestCase):
    def setUp(self):
        self._model = mock.create_autospec(PeriodicTableModel, instance=True)
        self.view = PeriodicTableView()
        self.presenter = PeriodicTablePresenter(self.view, self._model)
        self.presenter.is_selected = mock.Mock()
        self.mock_elem = mock.create_autospec(PeriodicTableItem, instance=True)
        self.mock_elem.symbol = mock.Mock()

        self.view.ptable = mock.create_autospec(silxPT, instance=True)
        self.view.ptable.getSelection = mock.Mock(return_value=self.mock_elem)
        self.view.ptable.isElementSelected = mock.Mock(return_value=True)

        self.view.on_table_lclicked = mock.Mock()
        self.view.on_table_rclicked = mock.Mock()
        self.view.on_table_changed = mock.Mock()

        self.view.unreg_on_table_lclicked = mock.Mock()
        self.view.unreg_on_table_rclicked = mock.Mock()
        self.view.unreg_on_table_changed = mock.Mock()

        self.presenter.view = self.view

    # checks if subsequent function is called on func()
    def check_second_func_called(self, register_func, signal_func):
        test_slot = mock.Mock()
        register_func(test_slot)
        assert signal_func.call_count == 1

    def test_register_table_lclicked(self):
        self.check_second_func_called(self.presenter.register_table_lclicked, self.view.on_table_lclicked)

    def test_unregister_table_lclicked(self):
        self.check_second_func_called(self.presenter.unregister_table_lclicked, self.view.unreg_on_table_lclicked)

    def test_register_table_rclicked(self):
        self.check_second_func_called(self.presenter.register_table_rclicked, self.view.on_table_rclicked)

    def test_unregister_table_rclicked(self):
        self.check_second_func_called(self.presenter.unregister_table_rclicked, self.view.unreg_on_table_rclicked)

    def test_register_table_changed(self):
        self.check_second_func_called(self.presenter.register_table_changed, self.view.on_table_changed)

    def test_unregister_table_changed(self):
        self.check_second_func_called(self.presenter.unregister_table_changed, self.view.unreg_on_table_changed)

    def test_selection(self):
        assert self.presenter.selection == self.mock_elem

    def test_is_selected(self):
        assert self.presenter.is_selected(mock.Mock())

    def test_select_element(self):
        self.check_second_func_called(self.presenter.select_element, self.view.ptable.setElementSelected)

    def test_add_elements(self):
        self.check_second_func_called(self.presenter.add_elements, self.view.ptable.setSelection)

    def test_set_buttons(self):
        self.presenter.model.peak_data = [self.mock_elem.symbol]
        self.view.ptable.elements = [self.mock_elem]
        self.presenter.set_buttons()
        assert self.view.ptable.silentSetElementSelected.call_count == 1
        assert self.view.ptable.enableElementButton.call_count == 1

    def test_set_peak_datafile(self):
        self.presenter.set_buttons = mock.Mock()
        test_filename = mock.Mock
        self.presenter.set_peak_datafile(test_filename)
        assert self.presenter.model.peak_data_file == test_filename


if __name__ == "__main__":
    unittest.main()
