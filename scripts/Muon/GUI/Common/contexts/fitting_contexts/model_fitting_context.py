# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import BasicFittingContext


class ModelFittingContext(BasicFittingContext):

    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(ModelFittingContext, self).__init__(allow_double_pulse_fitting)

        self._result_table_names: list = []

        self._x_parameters: list = []
        self._x_selected_index: int = None

        self._y_parameters: list = []
        self._y_selected_index: int = None
