# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from enum import Enum


class RowValid(Enum):
    invalid_for_all_runs = 0
    valid_for_some_runs = 1
    valid_for_all_runs = 2


class EAGroupingTabModel:
    """
    The model for the grouping tab should be shared between all widgets of the tab.
    It keeps a record of the groups defined for the current instance of the interface.

    groups should be of type EAGroup.
    """

    def __init__(self, context=None):
        self._context = context
        self._data = context.data_context
        self._groups = context.group_context
        self._gui_variables = context.gui_context

    @property
    def groups(self):
        return self._groups.groups

    @property
    def group_context(self):
        return self._groups

    @property
    def group_names(self):
        return self._groups.group_names

    @property
    def selected_groups(self):
        return self._groups.selected_groups

    def clear_groups(self):
        self._groups.clear()

    def clear_selected_groups(self):
        self._groups.clear_selected_groups()

    def clear(self):
        self.clear_groups()
        self.clear_selected_groups()

    def select_all_groups_to_analyse(self):
        self._groups.set_selected_groups_to_all()

    def remove_group_from_analysis(self, group):
        self._groups.remove_group_from_selected_groups(group)

    def add_group_to_analysis(self, group):
        self._groups.add_group_to_selected_groups(group)

    def add_group(self, group):
        assert isinstance(group, EAGroup)
        self._groups.add_new_group(group, self._data._loaded_data)

    def remove_groups_by_name(self, name_list):
        for name in name_list:
            self._groups.remove_group(name)

    def add_group_from_table(self, group):
        assert isinstance(group, EAGroup)
        self._groups.add_group(group)

    def reset_groups_to_default(self):
        if not self._context.current_runs:
            return "failed"

        self._groups.reset_group_to_default(self._data.current_workspace, self._data.instrument)
        return "success"

    def reset_selected_groups(self):
        self._groups.reset_selected_groups()

    def is_data_loaded(self):
        return self._data.is_data_loaded()

    def handle_rebin(self, **kwargs):
        self._context.handle_rebin(**kwargs)

    def show_all_groups(self):
        self._context.show_all_groups()
