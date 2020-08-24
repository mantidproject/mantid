# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import abc
from typing import List
from mantid.dataobjects import Workspace2D


class PlottingCanvasPresenterInterface(abc.ABC):

    @abc.abstractmethod
    def plot_workspaces(self, workspace_names: List[str], workspace_indices: List[int],
                        hold_on: bool, autoscale: bool):
        """Plots the input workspace names and indices in the figure window
        If hold_on is True the existing workspaces plotted in the figure are kept
        :param workspace_names: List of workspace names.
        :param workspace_indices: List of workspace indices.
        :param hold_on: Whether to keep the previous plots, boolean.
        :param autoscale: Whether to autoscale the graph, boolean.
        """
        pass

    @abc.abstractmethod
    def remove_workspace_names_from_plot(self, workspace_names: List[str]):
        """Removes the input workspace names from the plot
        :param workspace_names: List of workspace names
        """
        pass

    @abc.abstractmethod
    def remove_workspace_from_plot(self, workspace: Workspace2D):
        """Removes all references to the input workspace from the plot
        :param workspace: Input workspace 2D to remove
        """
        pass

    @abc.abstractmethod
    def replace_workspace_in_plot(self, workspace: Workspace2D):
        """Replace specified workspace in the plot with a new and presumably updated instance
        :param workspace: Input workspace 2D to remove
        """
        pass

    @abc.abstractmethod
    def create_tiled_plot(self, keys: List[str], tiled_by: str):
        """Creates a blank tiled plot specified by the keys and tiled by type
        :param keys: A list of keys which will be used to label and categorize each tile
        :param tiled_by: A string which states how the tiles are arranged (can be either by Groups or Runs)
        """
        pass

    @abc.abstractmethod
    def create_single_plot(self):
        """Creates a blank single plot"""
        pass

    @abc.abstractmethod
    def convert_plot_to_tiled_plot(self, keys: List[str], tiled_by: str):
        """Converts the current plot into a tiled plot specified by the keys and tiled by type
        In then replots the existing data on the new tiles
        :param keys: A list of keys which will be used to label and categorize each tile
        :param tiled_by: A string which states how the tiles are arranged (can be either by Groups or Runs)
        """
        pass

    @abc.abstractmethod
    def convert_plot_to_single_plot(self):
        """Converts the current plot into a single plot
        In then replots the existing data on the new tiles"""
        pass

    @abc.abstractmethod
    def get_plotted_workspaces_and_indices(self):
        """Returns the workspace names and indices which are plotted in the figure
        :return: A tuple of workspace_names and workspace indices"""
        pass

    @abc.abstractmethod
    def get_plot_axes(self):
        """Returns the matplotlib axes - needed for the external plot button
        :return: matplotlib axes instance"""
        pass

    @abc.abstractmethod
    def plot_guess_workspace(self, guess_ws_name: str):
        """Plots the guess workspace from a fit"""
        pass

    @abc.abstractmethod
    def get_plot_x_range(self):
        """Returns the x range of the first plot
        :return: a tuple contained the start and end ranges"""
        pass

    @abc.abstractmethod
    def set_plot_range(self, range : List[float]):
        """Sets the x range of all the plots"""
        pass
