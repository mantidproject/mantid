# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.fitting_contexts.model_fitting_context import ModelFittingContext
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel


class ModelFittingModel(BasicFittingModel):
    """
    The ModelFittingModel derives from BasicFittingModel.
    """

    def __init__(self, context: MuonContext, fitting_context: ModelFittingContext):
        """Initialize the GeneralFittingModel with emtpy fit data."""
        super(ModelFittingModel, self).__init__(context, fitting_context)

    @property
    def result_table_names(self) -> list:
        """Returns the names of the results tables loaded into the model fitting tab."""
        return self.fitting_context.result_table_names

    @result_table_names.setter
    def result_table_names(self, table_names: list) -> None:
        """Sets the names of the results tables loaded into the model fitting tab."""
        self.fitting_context.result_table_names = table_names

    @property
    def number_of_result_tables(self):
        """Returns the number of result tables which are held by the context."""
        return self.fitting_context.number_of_result_tables

    def get_workspace_names_to_display_from_context(self) -> list:
        """Returns the names of results tables to display in the view."""
        return self._check_data_exists(self.context.results_context.result_table_names)
