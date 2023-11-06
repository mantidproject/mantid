# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.features.add_grouping_workspaces import AddGroupingWorkspaces
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.corrections_tab_widget import CorrectionsTabWidget
from mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_widget import PhaseTabWidget
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.Transform.transform_widget import TransformWidget


class AddRawGroupingTest(unittest.TestCase):
    def setUp(self):
        self.GUI = mock.Mock()
        self.GUI.context = mock.MagicMock()
        self.GUI.context.do_grouping = mock.MagicMock()
        self.GUI.corrections_tab = mock.MagicMock(autospec=CorrectionsTabWidget)
        self.GUI.phase_tab = mock.MagicMock(autospec=PhaseTabWidget)

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.features.add_grouping_workspaces.GenericObserver")
    def test_get_features_success(self, observer):
        test = {}
        AddGroupingWorkspaces(self.GUI, test)
        observer.assert_called_once_with(self.GUI.context.do_grouping)

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.features.add_grouping_workspaces.GenericObserver")
    def test_add_observers_to_feature(self, observer):
        group_observer = mock.Mock()
        observer.return_value = group_observer

        add = AddGroupingWorkspaces(self.GUI, {})
        add.add_observers_to_feature(self.GUI)

        self.GUI.corrections_tab.corrections_tab_presenter.asymmetry_pair_and_diff_calculations_finished_notifier.add_subscriber.assert_called_once_with(
            group_observer
        )
        self.GUI.phase_tab.phase_table_presenter.calculation_finished_notifier.add_subscriber.assert_called_once_with(group_observer)
        self.GUI.phase_tab.phase_table_presenter.phase_table_calculation_complete_notifier.add_subscriber.assert_called_once_with(
            group_observer
        )

    @mock.patch("mantidqtinterfaces.Muon.GUI.Common.features.add_grouping_workspaces.GenericObserver")
    def test_add_observers_to_feature_with_frequency(self, observer):
        group_observer = mock.Mock()
        observer.return_value = group_observer
        self.GUI.transform = mock.MagicMock(autospec=TransformWidget)

        add = AddGroupingWorkspaces(self.GUI, {})
        add.add_observers_to_feature(self.GUI)

        self.GUI.corrections_tab.corrections_tab_presenter.asymmetry_pair_and_diff_calculations_finished_notifier.add_subscriber.assert_called_once_with(
            group_observer
        )
        self.GUI.phase_tab.phase_table_presenter.calculation_finished_notifier.add_subscriber.assert_called_once_with(group_observer)
        self.GUI.phase_tab.phase_table_presenter.phase_table_calculation_complete_notifier.add_subscriber.assert_called_once_with(
            group_observer
        )
        self.GUI.transform.new_data_observer.assert_called_once_with(group_observer)


if __name__ == "__main__":
    unittest.main()
