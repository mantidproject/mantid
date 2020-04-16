# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import abc
from typing import List
from qtpy import QtWidgets
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import WorkspacePlotInformation


# This metaclass is required in order to implement the (strict) interface for the QtWidget
class PlottingViewMeta(type(QtWidgets.QWidget), abc.ABCMeta):
    pass


class PlottingCanvasViewInterface(metaclass=PlottingViewMeta):

    @property
    @abc.abstractmethod
    def num_plotted_workspaces(self):
        """Number of workspaces plotted in the figure"""
        pass

    @property
    @abc.abstractmethod
    def number_of_axes(self):
        """Number of axes present in the figure"""
        pass

    @property
    @abc.abstractmethod
    def plotted_workspaces_and_indices(self):
        """Returns plotted workspaces and indices"""
        pass

    @abc.abstractmethod
    def create_new_plot_canvas(self, num_axes):
        """Creates a new blank plotting canvas"""
        pass

    @abc.abstractmethod
    def clear_all_workspaces_from_plot(self):
        """Clears all workspaces from the plot"""
        pass

    @abc.abstractmethod
    def add_workspaces_to_plot(self, workspace_plot_info_list: List[WorkspacePlotInformation]):
        """Add a list of workspaces to the plot - The workspaces are contained in a list PlotInformation
        The PlotInformation contains the workspace name, workspace index and target axis."""
        pass

    @abc.abstractmethod
    def remove_workspace_from_plot(self, workspace):
        """Remove a list of workspaces to the plot - The workspaces are contained in a list PlotInformation
           The PlotInformation contains the workspace name, workspace index and target axis."""
        pass

    @abc.abstractmethod
    def replace_specified_workspace_in_plot(self, workspace):
        """Replace specified workspace in the plot with a new and presumably updated instance"""
        pass

    @abc.abstractmethod
    def replot_workspace_with_error_state(self, workspace_name, with_errors: bool):
        """Replace specified workspace in the plot with a new and presumably updated instance"""
        pass

    @abc.abstractmethod
    def set_axis_xlimits(self, axis_number, xlims):
        """Set the xlimits of the specified axis"""
        pass

    @abc.abstractmethod
    def set_axis_ylimits(self, axis_number, ylims):
        """Set the ylimits of the specified axis"""
        pass

    @abc.abstractmethod
    def set_title(self, axis_number, title):
        """Set the title of the specified axis"""
        pass

    @abc.abstractmethod
    def get_axis_limits(self, axis_number):
        """Get the x and y limits if the specified axis"""
        pass
