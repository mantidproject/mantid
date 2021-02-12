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
        self._xlim_all = self._default_xlim
        self._ylim_all = self._default_ylim
        self._autoscale_all = False
        self._errors_all = False
        self._min_y_range = 2.0
        self._y_axis_margin = 20.

    def set_defaults(self, default_xlim:List[float], default_ylim:List[float]):
        self._default_xlim = default_xlim
        self._default_ylim = default_ylim
        self._xlim_all = self._default_xlim
        self._ylim_all = self._default_ylim

    @property
    def min_y_range(self):
        return self._min_y_range/2.

    @property
    def y_axis_margin(self):
        # stored as a percentage, but return decimal
        return self._y_axis_margin/100.

    @property
    def default_xlims(self):
        return self._default_xlim

    @property
    def default_ylims(self):
        return self._default_ylim

    def add_subplot(self, name:str, index:int):
        if name in (self._subplots.keys()):
            return
        self._subplots[name] = PlotEditContext()
        self._subplots[name].update_xlim(self._default_xlim)
        self._subplots[name].update_ylim(self._default_ylim)
        self._subplots[name].set_axis(index)

    def clear_subplots(self):
        for name in list(self._subplots.keys()):
            del self._subplots[name]

    """ methods for the All option """

    def update_xlim_all(self, values:List[float]):
        self._xlim_all = values

    @property
    def get_xlim_all(self)->List[float]:
        return self._xlim_all

    def update_ylim_all(self, values:List[float]):
        self._ylim_all = values

    @property
    def get_ylim_all(self)->List[float]:
        return self._ylim_all

    def set_autoscale_all(self, state:bool):
        self._autoscale_all = state
    
    @property
    def get_autoscale_all(self)->bool:
        return self._autoscale_all

    def set_error_all(self, state:bool):
        print("error",state)
        self._errors_all = state

    @property
    def get_error_all(self)->bool:
        return self._errors_all

    """ get/eidt values for a specific subplot"""
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
        self._subplots[name].update_error_state(state)

    def get_xlim(self, name:str)->List[float]:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_xlim()
        return self._subplots[name].get_xlim

    def get_ylim(self, name:str)->List[float]:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_ylim()
        return self._subplots[name].get_ylim

    def get_autoscale_state(self, name:str)->bool:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_autoscale_state
        return self._subplots[name].get_autoscale_state

    def get_error_state(self, name:str)->bool:
        if name == "All":
            return self._subplots[self._subplots.keys[0]].get_error_state
        return self._subplots[name].get_error_state

    def get_axis(self, name:str)->int:
        return self._subplots[name].axis

    def update_axis(self,name:str, index:int):
        if name not in self._subplots.keys():
            self.add_subplot(name, index)
        else:
            self._subplots[name].set_axis(index)