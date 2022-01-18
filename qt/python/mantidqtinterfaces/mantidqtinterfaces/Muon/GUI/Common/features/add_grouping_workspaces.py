# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.features.add_feature import AddFeature
from mantidqt.utils.observer_pattern import GenericObserver


class AddGroupingWorkspaces(AddFeature):
    """
    Add code to place workspaces into a workspace group
    """

    def __init__(self, GUI, _):
        super().__init__(GUI, _)

    def _get_features(self, _):
        return []

    def _add_features(self, GUI):
        # do grouping will move to context
        GUI.group_observer = GenericObserver(GUI.context.do_grouping)

    def add_observers_to_feature(self, GUI):
        # only do grouping when group/pair changes
        GUI.corrections_tab.corrections_tab_presenter.asymmetry_pair_and_diff_calculations_finished_notifier.\
            add_subscriber(GUI.group_observer)
        # phaseqaud finished -> do grouping
        GUI.phase_tab.phase_table_presenter.calculation_finished_notifier.add_subscriber(GUI.group_observer)
        GUI.phase_tab.phase_table_presenter.phase_table_calculation_complete_notifier.add_subscriber(GUI.group_observer)
        if hasattr(GUI, "transform"):
            GUI.transform.new_data_observer(GUI.group_observer)
