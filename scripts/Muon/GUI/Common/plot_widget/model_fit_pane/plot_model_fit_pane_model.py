# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_model import PlotFitPaneModel


class PlotModelFitPaneModel(PlotFitPaneModel):

    def __init__(self, context):
        super().__init__(context, "Model Fit Data")
        self.context.plot_panes_context[self.name].set_defaults([0., 1.0], [0.0, 1.0])

    def _create_workspace_label(self, workspace_name, index):
        result_table_name = "ResultTable"
        fit_label = self._get_fit_label(workspace_name, index)
        return f"{result_table_name};{fit_label}"
