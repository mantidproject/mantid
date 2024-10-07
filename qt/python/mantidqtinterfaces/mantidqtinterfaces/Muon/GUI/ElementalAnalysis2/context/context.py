# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable
from mantidqtinterfaces.Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from mantidqtinterfaces.Muon.GUI.Common import thread_model
from mantid.simpleapi import CloneWorkspace
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, remove_ws_if_present
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import rebin_ws
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_ADS_observer import MuonADSObserver

REBINNED_FIXED_WS_SUFFIX = "_EA_Rebinned_Fixed"
REBINNED_VARIABLE_WS_SUFFIX = "_EA_Rebinned_Variable"


class ElementalAnalysisContext(object):
    def __init__(
        self,
        data_context,
        ea_group_context=None,
        muon_gui_context=None,
        plot_panes_context=None,
        error_notifier=None,
        workspace_suffix=" EA",
    ):
        self._window_title = "Elemental Analysis 2"
        self.data_context = data_context
        self._gui_context = muon_gui_context
        self._group_context = ea_group_context
        self._plot_panes_context = plot_panes_context
        self.workspace_suffix = workspace_suffix
        self.ads_observer = MuonADSObserver(
            delete_callback=self.remove_workspace,
            clear_callback=self.clear_context,
            replace_callback=self.workspace_replaced,
            delete_group_callback=self.remove_workspace,
        )

        self.update_view_from_model_notifier = GenericObservable()
        self.update_plots_notifier = GenericObservable()
        self.deleted_plots_notifier = GenericObservable()
        self.calculation_started_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()
        self.error_notifier = error_notifier

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
        workspace_name = str(workspace)
        if workspace_name not in self.group_context.group_names:
            self.group_context.remove_workspace_from_group(workspace_name)
            self.deleted_plots_notifier.notify_subscribers(workspace)
            if "Rebin" not in workspace_name:
                self.update_view_from_model_notifier.notify_subscribers(workspace_name)
            return
        self.group_context.remove_group(workspace_name)
        self.update_view_from_model_notifier.notify_subscribers(workspace_name)
        self.deleted_plots_notifier.notify_subscribers(workspace)

    def clear_context(self):
        self.data_context.clear()
        self.group_context.clear()

    def workspace_replaced(self, workspace):
        workspace_name = str(workspace)
        if workspace_name not in self.group_context.group_names:
            return
        self.update_plots_notifier.notify_subscribers(workspace)

    def handle_calculation_started(self):
        self.calculation_started_notifier.notify_subscribers()

    def calculation_success(self):
        self.calculation_finished_notifier.notify_subscribers()

    def handle_calculation_error(self, error):
        self.calculation_finished_notifier.notify_subscribers()
        if self.error_notifier:
            error_message = "Unexpected error occurred during Rebin: " + str(error)
            self.error_notifier.notify_subscribers(error_message)

    def _run_rebin(self, name, rebin_type, params):
        rebined_run_name = None
        if rebin_type == "Fixed":
            rebin_index = 1
            rebin_option = "Steps: " + str(params) + " KeV"
            rebined_run_name = str(name) + REBINNED_FIXED_WS_SUFFIX
        if rebin_type == "Variable":
            rebin_index = 2
            rebin_option = "Bin Boundaries: " + str(params)
            rebined_run_name = str(name) + REBINNED_VARIABLE_WS_SUFFIX

        remove_ws_if_present(rebined_run_name)
        raw_workspace = self.group_context[name].get_counts_workspace_for_run()

        CloneWorkspace(InputWorkspace=raw_workspace, OutputWorkspace=rebined_run_name)
        rebin_ws(rebined_run_name, params)

        workspace = retrieve_ws(rebined_run_name)
        group_workspace = retrieve_ws(self.group_context[name].run_number)
        group_workspace.addWorkspace(workspace)
        group = self.group_context[name]
        group.update_workspaces(str(workspace), rebin=True, rebin_index=rebin_index, rebin_option=rebin_option)
        self.update_plots_notifier.notify_subscribers(workspace)

    def handle_rebin(self, name, rebin_type, rebin_param):
        self.rebin_model = ThreadModelWrapper(lambda: self._run_rebin(name, rebin_type, rebin_param))
        self.rebin_thread = thread_model.ThreadModel(self.rebin_model)
        self.rebin_thread.threadWrapperSetUp(
            on_thread_start_callback=self.handle_calculation_started,
            on_thread_end_callback=self.calculation_success,
            on_thread_exception_callback=self.handle_calculation_error,
        )
        self.rebin_thread.start()

    def show_all_groups(self):
        pass
