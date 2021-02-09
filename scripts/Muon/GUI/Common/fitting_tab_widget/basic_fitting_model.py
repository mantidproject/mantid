# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.fitting_context import FitInformation

from typing import List, NamedTuple

FDA_GUESS_WORKSPACE = "__frequency_domain_analysis_fitting_guess"
MA_GUESS_WORKSPACE = "__muon_analysis_fitting_guess"
FDA_SUFFIX = " FD"
MA_SUFFIX = " MA"


class FitPlotInformation(NamedTuple):
    input_workspaces: List[str]
    fit: FitInformation


class BasicFittingModel:
    def __init__(self, context):
        self.context = context

        self.function_name = ""
        #self._grppair_index = {}
        self.fitting_options = {}
        self.ws_fit_function_map = {}
