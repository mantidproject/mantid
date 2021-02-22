# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel


class TFAsymmetryFittingModel(GeneralFittingModel):
    """
    The TFAsymmetryFittingModel derives from GeneralFittingModel. It adds the ability to do TF Asymmetry fitting.
    """

    def __init__(self, context: MuonContext, is_frequency_domain: bool = False):
        """Initialize the TFAsymmetryFittingModel with emtpy fit data."""
        super(TFAsymmetryFittingModel, self).__init__(context, is_frequency_domain)
