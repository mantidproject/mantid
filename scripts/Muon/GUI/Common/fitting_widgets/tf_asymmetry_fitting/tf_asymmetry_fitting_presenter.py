# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_model import TFAsymmetryFittingModel
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView


class TFAsymmetryFittingPresenter(GeneralFittingPresenter):
    """
    The TFAsymmetryFittingPresenter has a TFAsymmetryFittingView and TFAsymmetryFittingModel and derives from
    GeneralFittingPresenter.
    """

    def __init__(self, view: TFAsymmetryFittingView, model: TFAsymmetryFittingModel):
        """Initialize the TFAsymmetryFittingPresenter. Sets up the slots and event observers."""
        super(TFAsymmetryFittingPresenter, self).__init__(view, model)

    def initialize_model_options(self) -> None:
        """Returns the fitting options to be used when initializing the model."""
        super().initialize_model_options()
