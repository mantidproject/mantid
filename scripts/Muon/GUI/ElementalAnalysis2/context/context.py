# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model
from mantid.simpleapi import Rebin
from Muon.GUI.Common import message_box
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, remove_ws_if_present

REBINNED_FIXED_WS_SUFFIX = "_EA_Rebinned_Fixed"
REBINNED_VARIABLE_WS_SUFFIX = "_EA_Rebinned_Variable"


class ElementalAnalysisContext(object):

    def __init__(self, data_context, ea_group_context=None, muon_gui_context=None, plot_panes_context=None, workspace_suffix=' EA'):
        self._window_title = "Elemental Analysis 2"
        self.data_context = data_context
        self._gui_context = muon_gui_context
        self._group_context = ea_group_context
        self._plot_panes_context = plot_panes_context
        self.workspace_suffix = workspace_suffix

        self.update_view_from_model_notifier = GenericObservable()
        self.update_plots_notifier = GenericObservable()
        self.deleted_plots_notifier = GenericObservable()
        self.calculation_started_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()

    @property
    def name(self):
        return self._window_title

    @property
    def gui_context(self):
        return self._gui_context

    @property
    def plot_panes_context(self):
        return self._plot_panes_context

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
        self.group_context.remove_group(workspace_name)
        self.update_view_from_model_notifier.notify_subscribers(workspace_name)
        self.update_plots_notifier.notify_subscribers(workspace_name)

    def clear_context(self):
        self.data_context.clear()
        self.group_context.clear()

    def workspace_replaced(self, workspace):
        self.update_plots_notifier.notify_subscribers(workspace)

    def handle_calculation_started(self):
        self.calculation_started_notifier.notify_subscribers()

    def calculation_success(self):
        self.calculation_finished_notifier.notify_subscribers()

    def handle_calculation_error(self, error):
        self.calculation_finished_notifier.notify_subscribers()
        message_box.warning(str(error), None)

    def _run_rebin(self, name, rebin_type, params):
        rebined_run_name = None
        if rebin_type == "Fixed":
            rebined_run_name = str(name) + REBINNED_FIXED_WS_SUFFIX
        if rebin_type == "Variable":
            rebined_run_name = str(name) + REBINNED_VARIABLE_WS_SUFFIX

        remove_ws_if_present(rebined_run_name)

        workspace = Rebin(InputWorkspace=name, OutputWorkspace=rebined_run_name, Params=params)
        group = retrieve_ws(name.split(";")[0])
        group.addWorkspace(workspace)
        self.group_context[name].update_workspaces(name, workspace, rebin=True)

    def handle_rebin(self, name, rebin_type, rebin_param):
        self.rebin_model = ThreadModelWrapper(lambda: self._run_rebin(name, rebin_type, rebin_param))
        self.rebin_thread = thread_model.ThreadModel(self.rebin_model)
        self.rebin_thread.threadWrapperSetUp(self.handle_calculation_started,
                                             self.calculation_success,
                                             self.handle_calculation_error)
        self.rebin_thread.start()

    def show_all_groups(self):
        pass
