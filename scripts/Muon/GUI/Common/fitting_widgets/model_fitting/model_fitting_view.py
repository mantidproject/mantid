# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_data_selector_view import ModelFittingDataSelectorView

from qtpy.QtWidgets import QWidget


class ModelFittingView(BasicFittingView):
    """
    The ModelFittingView derives from the BasicFittingView. It adds the ModelFittingDataSelectorView to the widget.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the ModelFittingView, and adds the ModelFittingDataSelectorView widget."""
        super(ModelFittingView, self).__init__(parent)

        self.model_fitting_data_selector = ModelFittingDataSelectorView(self)
        self.general_fitting_options_layout.addWidget(self.model_fitting_data_selector)

        # Hide the workspace selector which is used to store the generated matrix workspaces
        self.workspace_selector.hide()

    def update_result_table_names(self, table_names: list) -> None:
        """Update the data in the parameter display combo box."""
        self.model_fitting_data_selector.update_result_table_names(table_names)
