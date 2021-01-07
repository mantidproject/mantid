# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.context.data_context import DataContext
from mantidqt.utils.observer_pattern import Observable


class ElementalAnalysisContext(object):

    def __init__(self ,ea_group_context=None, muon_gui_context=None, workspace_suffix=' EA'):
        self._window_title = "Elemental Analysis 2"
        self.data_context = DataContext()
        self._gui_context = muon_gui_context
        self._group_context = ea_group_context
        self.workspace_suffix = workspace_suffix

        self.update_view_from_model_notifier = Observable()
        self.update_plots_notifier = Observable()

    @property
    def name(self):
        return self._window_title

    @property
    def gui_context(self):
        return self._gui_context

    @property
    def group_context(self):
        return self._group_context

    def update_current_data(self):
        if len(self.data_context.current_runs) > 0:

            if not self.group_context.groups:
                self.group_context.reset_group_to_default(self.data_context._loaded_data)

            else:
                self.group_context.add_new_group(self.group_context.groups, self.data_context._loaded_data)
        else:
            self.data_context.clear()

    def remove_workspace(self, workspace):
        # required as the renameHandler returns a name instead of a workspace.
        if isinstance(workspace, str):
            workspace_name = workspace
        else:
            workspace_name = workspace.name()

        self.data_context.remove_workspace_by_name(workspace_name)
        self.group_context.remove_workspace_by_name(workspace_name)
        self.gui_context.remove_workspace_by_name(workspace_name)
        self.update_view_from_model_notifier.notify_subscribers(workspace_name)

    def clear_context(self):
        self.data_context.clear()
        self.group_context.clear()

    def workspace_replaced(self, workspace):
        self.update_plots_notifier.notify_subscribers(workspace)
