# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_model import PlotFitPaneModel
from Muon.GUI.Common.ADSHandler.workspace_naming import (FFT_STR, MAXENT_STR,
                                                         get_fft_component_from_workspace_name,
                                                         get_group_or_pair_from_name,
                                                         get_run_numbers_as_string_from_workspace_name)


class PlotFreqFitPaneModel(PlotFitPaneModel):

    def __init__(self, context):
        super().__init__(context,"Frequency Data")
        end_x = self.context.default_end_x
        self.context.plot_panes_context[self.name].set_defaults([0.,end_x], [0.0, 1.0])

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_numbers_as_string_from_workspace_name(workspace_name, self.context.data_context.instrument))
        instrument = self.context.data_context.instrument
        fit_label = self._get_fit_label(workspace_name, index)
        freq_label = self._get_freq_label(workspace_name)
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return f"{instrument}{run};{group}{fit_label}{freq_label}"
        if self.context.plot_panes_context[self.name].settings.is_tiled_by == "Group/Pair":
            return f"{run}{fit_label}{freq_label}"
        else:
            return f"{group}{fit_label}{freq_label}"

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index = 0):
        return 0

    @staticmethod
    def _get_freq_label(workspace_name):
        label = ''
        if FFT_STR in workspace_name:
            label = f";{get_fft_component_from_workspace_name(workspace_name)}"
        elif MAXENT_STR in workspace_name:
            label = f';{MAXENT_STR}'
        return label
