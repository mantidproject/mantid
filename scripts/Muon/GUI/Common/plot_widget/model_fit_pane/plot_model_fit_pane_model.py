# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_model import PlotFitPaneModel


class PlotModelFitPaneModel(PlotFitPaneModel):

    def __init__(self, context):
        super().__init__(context, "Model Data")
        self.context.plot_panes_context[self.name].set_defaults([0.0, 1.0], [0.0, 1.0])
        self.context.plot_panes_context[self.name].set_autoscale_all(True)
        self.context.plot_panes_context[self.name].set_error_all(True)

    def _create_workspace_label(self, workspace_name, index):
        return f"{workspace_name}{self._get_fit_label(index)}"

    @staticmethod
    def _get_fit_label(index):
        if index == 1:
            return ";Calc"
        elif index == 2:
            return ";Diff"
        else:
            return ""
