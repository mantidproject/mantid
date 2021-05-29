# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_presenter import BasicFittingPresenter
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_model import ModelFittingModel
from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_view import ModelFittingView


class ModelFittingPresenter(BasicFittingPresenter):
    """
    The ModelFittingPresenter has a ModelFittingView and ModelFittingModel and derives from BasicFittingPresenter.
    """

    def __init__(self, view: ModelFittingView, model: ModelFittingModel):
        """Initialize the ModelFittingPresenter. Sets up the slots and event observers."""
        super(ModelFittingPresenter, self).__init__(view, model)

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
