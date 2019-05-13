# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from qtpy import QtWidgets

from mantid import ConfigService
from mantid.api import FileFinder
from mantid.py3compat import mock

import Muon.GUI.Common.utilities.load_utils as load_utils
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_model import HomeGroupingWidgetModel
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_presenter import HomeGroupingWidgetPresenter
from Muon.GUI.Common.home_grouping_widget.home_grouping_widget_view import HomeGroupingWidgetView
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.observer_pattern import Observer
from Muon.GUI.Common.test_helpers import mock_widget
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


def perform_psi_file_finder(self):
    file_path = FileFinder.findRuns('deltat_tdc_dolly_1529.bin')[0]
    ws, run, filename, is_psi_data = load_utils.load_workspace_from_filename(file_path)
    self.assert_(is_psi_data)
    self.data_context._loaded_data.remove_data(run=run)
    self.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='PSI')
    self.data_context.current_runs = [[1529]]

    self.context.data_context._instrument = "PSI"
    self.context.update_current_data()
    test_pair = MuonPair('test_pair1', 'Forw', 'Back', alpha=0.75)
    self.group_context.add_pair(pair=test_pair)
    self.presenter.update_group_pair_list()


def perform_musr_file_finder(self):
    ConfigService['default.instrument'] = 'MUSR'
    file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
    ws, run, filename, is_psi_data = load_utils.load_workspace_from_filename(file_path)
    self.assert_(not is_psi_data)
    self.data_context._loaded_data.remove_data(run=run)
    self.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
    self.data_context.current_runs = [[22725]]

    self.context.data_context._instrument = "MUSR"
    self.context.update_current_data()
    test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
    self.group_context.add_pair(pair=test_pair)
    self.presenter.update_group_pair_list()


class HomeTabGroupingPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.obj = QtWidgets.QWidget()
        setup_context_for_tests(self)
        self.gui_context['RebinType'] = 'None'
        self.view = HomeGroupingWidgetView(self.obj)
        self.model = HomeGroupingWidgetModel(self.context)
        self.presenter = HomeGroupingWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.MagicMock()
        self.view.instrument_changed_warning = mock.MagicMock(return_value=1)

    def tearDown(self):
        self.obj = None

    def test_group_pair_selector_contains_groups_and_pairs(self):
        self.context.data_context.current_data["workspace"] = mock.MagicMock()
        perform_musr_file_finder(self)
        self.assertEqual(self.view.grouppair_selector.itemText(0), 'top')
        self.assertEqual(self.view.grouppair_selector.itemText(1), 'bkwd')
        self.assertEqual(self.view.grouppair_selector.itemText(2), 'bottom')
        self.assertEqual(self.view.grouppair_selector.itemText(3), 'fwd')
        self.assertEqual(self.view.grouppair_selector.itemText(4), 'test_pair')

    def test_that_changeing_to_pair_displays_alpha(self):
        perform_musr_file_finder(self)
        self.assertTrue(self.view.alpha_label_2.isHidden())
        self.assertTrue(self.view.alpha_edit.isHidden())

        self.view.grouppair_selector.setCurrentIndex(4)

        self.assertFalse(self.view.alpha_label_2.isHidden())
        self.assertFalse(self.view.alpha_edit.isHidden())
        self.assertEqual(self.view.alpha_edit.text(), '0.75')

    def test_that_changeing_alpha_correctly_updates_model(self):
        perform_musr_file_finder(self)
        self.view.grouppair_selector.setCurrentIndex(4)
        observer = Observer()
        observer.update = mock.MagicMock()
        self.presenter.pairAlphaNotifier.add_subscriber(observer)

        self.view.alpha_edit.setText('0.87')
        self.view.alpha_edit.editingFinished.emit()

        self.assertEqual(self.model.get_alpha('test_pair'), 0.87)
        observer.update.assert_called_once_with(self.presenter.pairAlphaNotifier, None)

    def test_period_changes_are_propogated_to_model(self):
        perform_musr_file_finder(self)
        self.model.number_of_periods = mock.MagicMock(return_value=5)

        self.view.summed_period_edit.setText('1, 3, 5')
        self.view.subtracted_period_edit.setText('2, 4')
        self.view.summed_period_edit.editingFinished.emit()

        self.view.warning_popup.assert_not_called()
        self.assertEqual(self.model.get_summed_periods(), [1,3,5])
        self.assertEqual(self.model.get_subtracted_periods(), [2, 4])

    def test_invalid_periods_are_removed_and_warning_given(self):
        perform_musr_file_finder(self)
        self.model.number_of_periods = mock.MagicMock(return_value=5)

        self.view.summed_period_edit.setText('1, 3, 5')
        self.view.subtracted_period_edit.setText('2, 4, 6')
        self.view.summed_period_edit.editingFinished.emit()

        self.view.warning_popup.assert_called_once_with('The following periods are invalid : 6')
        self.assertEqual(self.model.get_summed_periods(), [1,3,5])
        self.assertEqual(self.model.get_subtracted_periods(), [2, 4])

    def test_psi_specific_changes(self):
        perform_psi_file_finder(self)

        self.assertEqual(self.view.grouppair_selector.itemText(0), 'Forw')
        self.assertEqual(self.view.grouppair_selector.itemText(1), 'Back')
        self.assertEqual(self.view.grouppair_selector.itemText(2), 'Left')
        self.assertEqual(self.view.grouppair_selector.itemText(3), 'Rite')
        self.assertEqual(self.view.grouppair_selector.itemText(4), 'test_pair1')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)