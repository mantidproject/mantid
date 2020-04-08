# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import abc
from typing import List


class PlottingCanvasPresenterInterface(object):
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def plot_workspaces(self, workspace_names: List[str], workspace_indices: List[int],
                               overplot: bool, autoscale : bool):
        pass

    @abc.abstractmethod
    def remove_workspace_from_plot(self, workspace):
        pass

    @abc.abstractmethod
    def replace_workspace_in_plot(self, workspace):
        pass

    @abc.abstractmethod
    def create_tiled_plot(self, keys, tiled_by): pass

    @abc.abstractmethod
    def create_single_plot(self): pass

    @abc.abstractmethod
    def convert_plot_to_tiled_plot(self, keys: List[str], tiled_by: str):
        pass

    @abc.abstractmethod
    def convert_plot_to_single_plot(self):
        pass

    @abc.abstractmethod
    def get_plotted_workspaces_and_indices(self):
        pass
