# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List


class PlotEditContext(object):

    def __init__(self):
        self._xlim = [0.0, 15.]
        self._ylim = [-0.3, 0.3]
        self._plot_index = 0
        self._errors = False
        self._auto = False

    def update_xlim(self, values: List[float]):
        self._xlim = values

    def update_ylim(self, values: List[float]):
        self._ylim = values

    def update_error_state(self, state: bool):
        self._errors = state

    def update_autoscale_state(self, state: bool):
        self._auto = state

    def set_axis(self,index:int):
        self._plot_index = index

    @property
    def axis(self)->int:
        return self._plot_index

    @property
    def get_xlim(self) -> List[float]:
        return self._xlim

    @property
    def get_ylim(self) -> List[float]:
        return self._ylim

    @property
    def get_error_state(self) -> bool:
        return self._errors

    @property
    def get_autoscale_state(self) -> bool:
        return self._auto
