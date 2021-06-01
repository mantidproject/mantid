# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.fitting_widgets.basic_fitting.workspace_selector_view import WorkspaceSelectorView

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "model_fitting_data_selector.ui")


class ModelFittingDataSelectorView(ui_form, base_widget):
    """
    The ModelFittingDataSelectorView includes the cyclic results table data selector, and two combo boxes to select X
    and Y data.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the ModelFittingDataSelectorView."""
        super(ModelFittingDataSelectorView, self).__init__(parent)
        self.setupUi(self)

        self.result_table_selector = WorkspaceSelectorView(self)
        self.result_table_selector.set_workspace_combo_box_label("Results table")
        self.result_table_selector_layout.addWidget(self.result_table_selector)

    def update_result_table_names(self, table_names: list) -> None:
        """Update the data in the parameter display combo box."""
        self.result_table_selector.update_dataset_name_combo_box(table_names)
