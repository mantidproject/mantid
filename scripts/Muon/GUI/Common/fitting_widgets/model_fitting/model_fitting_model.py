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
