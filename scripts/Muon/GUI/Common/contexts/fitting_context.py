# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division)

from Muon.GUI.Common.observer_pattern import Observable


class FitInformation(object):
    """Data-object encapsulating a single fit"""

    def __init__(self, parameter_workspace, fit_function_name,
                 input_workspace, output_workspace_names):
        """
        :param parameter_workspace: The workspace wrapper
        that contains all of the parameters from the fit
        :param fit_function_name: The name of the function used
        :param input_workspace: The name of the workspace containing
        the original data
        """
        self.parameter_workspace = parameter_workspace
        self.fit_function_name = fit_function_name
        self.input_workspace = input_workspace
        self.output_workspace_names = output_workspace_names

    @property
    def parameter_name(self):
        """Returns the name of the parameter workspace"""
        return self.parameter_workspace.workspace_name

    def __eq__(self, other):
        """Objects are equal if each member is equal to the other"""
        return self.parameter_name == other.parameter_name and \
            self.fit_function_name == other.fit_function_name and \
            self.input_workspace == other.input_workspace and \
            self.output_workspace_names == other.output_workspace_names


class FittingContext(object):
    """Context specific to fitting.

    It holds details are any fits performed including:
       - input workspaces
       - parameters
       - function names
    """

    def __init__(self):
        self.fit_list = []
        # Register callbacks with this object to observe when new fits
        # are added
        self.new_fit_notifier = Observable()

    def __len__(self):
        """
        :return: The number of fits in the list
        """
        return len(self.fit_list)

    def add_fit_from_values(self, parameter_workspace, fit_function_name,
                            input_workspace, output_workspace_names):
        """
        Add a new fit information object based on the raw values.
        See FitInformation constructor for details are arguments
        """
        self.add_fit(
            FitInformation(parameter_workspace, fit_function_name,
                           input_workspace, output_workspace_names))

    def add_fit(self, fit):
        """
        Add a new fit to the context. Subscribers are notified of the update.
        :param fit: A new FitInformation object
        """
        if fit in self.fit_list:
            return
        self.fit_list.append(fit)
        self.new_fit_notifier.notify_subscribers()

    def fit_function_names(self):
        """
        :return: a list of unique function names used in the fit
        """
        return list(set([fit.fit_function_name for fit in self.fit_list]))

    def find_fits_for_function(self, fit_function_name):
        """
        Find the fits in the list whose function name matches
        :param fit_function_name: The name of the function
        :return: A list of any matching fits
        """
        return [
            fit for fit in self.fit_list
            if fit.fit_function_name == fit_function_name
        ]

    def find_output_workspaces_for_input_workspace_name(self, input_workspace_name):
        """
        Find the fits in the list whose input workspace matches
        :param input_workspace_name: The name of the input_workspace
        :return: A list of matching fits
        """
        workspace_list = []
        for fit in self.fit_list:
            if type(fit.input_workspace) == list:
                for index, workspace in enumerate(fit.input_workspace):
                    if workspace == input_workspace_name:
                        workspace_list.append(fit.output_workspace_names[index])
            else:
                if input_workspace_name == fit.input_workspace:
                    workspace_list.append(fit.output_workspace_names[0])
        return workspace_list
