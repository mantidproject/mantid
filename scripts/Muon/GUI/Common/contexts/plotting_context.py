# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.Common.contexts.plot_edit_context import PlotEditContext
from typing import List
from enum import Enum


PlotModeStrings = ['Plot Data', 'Plot Fits']


class PlotMode(Enum):
    Data = 0
    Fitting = 1

    def __str__(self):
        return PlotModeStrings[self.value]


class PlottingContext(object):
    def __init__(self):
        self._subplots = {}
        self._default_xlim = [-0.1,0.1]
        self._default_ylim = [-10,10]

    def set_defaults(self, default_xlim:List[float], default_ylim:List[float]):
        self._default_xlim = default_xlim
        self._default_ylim = default_ylim

    @property
    def default_xlims(self):
        return self._default_xlim
    @property
    def default_ylims(self):
        return self._default_ylim

    def add_subplot(self, name:str):
        self._subplots[name] = PlotEditContext()
        self._subplots[name].set_xlim(self._default_xlim)
        self._subplots[name].set_ylim(self._default_ylim)

    def remove_subplot(self,name:str):
        if name not in list(self._subplots.keys()):
            return
        del self._subplots[name]

    def update_xlim(self, name:str, values:List[float]):
        if name == "All":
            for subplot in list(self._subplots.keys()):
                self._subplots[subplot].update_xlim(values)
            return
        self._subplots[name].update_xlim(values)

    def update_ylim(self, name:str, values:List[float]):
        if name == "All":
            for subplot in list(self._subplots.keys()):
                self._subplots[subplot].update_ylim(values)
            return
        self._subplots[name].update_ylim(values)

    def update_autoscale_state(self, name:str, state:bool):
        if name == "All":
            for subplot in list(self._subplots.keys()):
                self._subplots[subplot].update_autoscale_state(state)
            return
        self._subplots[name].update_autoscale_state(state)

    def update_error_state(self, name:str, state:bool):
        if name == "All":
            for subplot in list(self._subplots.keys()):
                self._subplots[subplot].update_error_state(state)
            return
        self._subplots[name].update_autoscale_state(state)

    @property
    def get_xlim(self, name:str)->List[float]:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_xlim()
        return self.subplots[name].get_xlim()

    @property
    def get_ylim(self, name:str)->List[float]:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_ylim()
        return self.subplots[name].get_ylim()

    @property
    def get_autoscale_state(self, name:str)->bool:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_autoscale_state()
        return self.subplots[name].get_autoscale_state()

    @property
    def get_error_state(self, name:str)->bool:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_error_state()
        return self.subplots[name].get_error_state()

    @property
    def get_subplot_names()->List[str]:
        return list(self._subplots.keys())