# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import OrderedDict
import re

import numpy as np

from mantid.api import WorkspaceGroup
from mantidqt.utils.observer_pattern import Observable
from Muon.GUI.Common.ADSHandler.ADS_calls import add_ws_to_ads
from Muon.GUI.Common.ADSHandler.workspace_naming import create_fitted_workspace_name
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

# Magic values for names of columns in the fit parameter table
NAME_COL = 'Name'
VALUE_COL = 'Value'
ERRORS_COL = 'Error'

GLOBAL_PARAM_PREFIX_RE = re.compile(r'^f\d+\.')


def is_same_parameter(prefixed_name, unprefixed_name):
    """
    Check if two parameters are actually referring to the same parameter name.
    :param prefixed_name: A name such as f0.Lambda from the result of a fit
    :param unprefixed_name: A bare parameter name from a function, e.g. Lambda
    :return: True if these refer to the same parameter, False otherwise
    """
    return prefixed_name.endswith(unprefixed_name)


def _create_unique_param_lookup(parameter_workspace, global_parameters):
    """
    Create a dictionary with the parameter names as keys where each
    value is a 2-tuple of (value, error). Only unique parameters are
    included, i.e if parameters are marked global then only 1 parameter
    is kept.

    :param parameter_workspace: A raw parameter table from Fit
    :param global_parameters: A list of global parameters(if any)
    """

    def is_in(unique_params, param_name):
        # return 2-tuple(already exists, is_global)
        if not global_parameters:
            return False, False

        # Find the global parameter than matches this one
        global_name = None
        for name in global_parameters:
            if is_same_parameter(param_name, name):
                global_name = name
                break
        if global_name is None:
            # param_name is not in globals
            return False, False

        # Do we have this parameter already?
        for unique_name in unique_params.keys():
            if is_same_parameter(unique_name, global_name):
                return True, True

        return False, True

    unique_params = OrderedDict()
    default_table = parameter_workspace.workspace
    for row in default_table:
        name = row[NAME_COL]
        exists, is_global = is_in(unique_params, name)
        if not exists:
            parameter = Parameter(name, row[VALUE_COL], row[ERRORS_COL],
                                  is_global)
            unique_params[parameter.pretty_name] = parameter

    return _move_globals_to_front(unique_params)


def _move_globals_to_front(unique_params):
    """
    Move the global parameters to the front of the list
    :param unique_params: An list containing the current parameters in the original order
    :return: The updated parameters list reordered
    """
    return OrderedDict(
        sorted(unique_params.items(), key=lambda x: not x[1].is_global))


class Parameter(object):
    """Hold single parameter from a fit"""

    def __init__(self, raw_name, value, error, isglobal):
        """
        :param raw_name: The raw name of the parameter from Fit. The first "fx." prefix
        is stripped if
        :param value: The value of the parameter
        :param error: The error value on the parameter
        :param isglobal: True if the parameter was global across the fit
        """
        self._value = value
        self._error = error
        self._isglobal = isglobal

        if isglobal:
            self._name = GLOBAL_PARAM_PREFIX_RE.sub("", raw_name)
        else:
            self._name = raw_name

    @property
    def pretty_name(self):
        return self._name

    @property
    def value(self):
        return self._value

    @property
    def error(self):
        return self._error

    @property
    def is_global(self):
        return self._isglobal


class FitParameters(object):
    """Data-object encapsulating fit parameters for a single fit"""

    def __init__(self, parameter_workspace, global_parameters=None):
        """
        Initialize the fit parameters from a workspace
        :param parameter_workspace: Assumed to be a table workspace output from Fit
        """
        self._parameter_workspace = parameter_workspace
        self._global_parameters = global_parameters if global_parameters is not None else []
        self._unique_params = _create_unique_param_lookup(
            parameter_workspace, global_parameters)

    def __eq__(self, other):
        return self._parameter_workspace == other._parameter_workspace and \
            self._global_parameters == other._global_parameters

    def __ne__(self, other):
        return not self == other

    def __len__(self):
        """Returns the number of unique parameters"""
        return len(self._unique_params)

    def names(self):
        """Returns a list of names of parameters. Note that any global parameters are first in the list"""
        return list(self._unique_params.keys())

    def value(self, name):
        """Return the value of a given parameter"""
        return self._unique_params[name].value

    def error(self, name):
        """Return the error associated with a given parameter"""
        return self._unique_params[name].error

    @property
    def parameter_workspace_name(self):
        """
        :return: The name of the raw parameter workspace
        """
        return self._parameter_workspace.workspace_name

    @property
    def global_parameters(self):
        """
        :return: The set of global parameters
        """
        return self._global_parameters


class FitInformation(object):
    """Data-object encapsulating a single fit"""

    def __init__(self,
                 input_workspace_names: list,
                 fit_function_name: str,
                 output_workspaces: list,
                 parameter_workspace: StaticWorkspaceWrapper,
                 covariance_workspace: StaticWorkspaceWrapper,
                 global_parameters: list = None,
                 tf_asymmetry_fit: bool = False):
        """
        :param input_workspace_names: A list of input workspace names containing the original data.
        :param fit_function_name: The name of the function used.
        :param output_workspaces: A list of StaticWorkspaceWrapper's containing the output workspaces.
        :param parameter_workspace: A StaticWorkspaceWrapper containing the parameter workspace.
        :param covariance_workspace: A StaticWorkspaceWrapper containing the covariance workspace.
        :param global_parameters: An optional list of global parameters that were tied together during the fit.
        :param tf_asymmetry_fit: An optional flag indicating whether the data is from a TF Asymmetry fit or not.
        """
        self.input_workspaces: list = input_workspace_names
        self.fit_function_name: str = fit_function_name
        self.output_workspaces: list = output_workspaces
        self.parameter_workspace: StaticWorkspaceWrapper = parameter_workspace
        self.covariance_workspace: StaticWorkspaceWrapper = covariance_workspace
        self.tf_asymmetry_fit: bool = tf_asymmetry_fit

        self._fit_parameters = FitParameters(self.parameter_workspace, global_parameters)

    def __eq__(self, other):
        """Objects are equal if each member is equal to the other"""
        return self.input_workspaces == other.input_workspaces and \
            self.fit_function_name == other.fit_function_name and \
            self.output_workspace_names() == other.output_workspace_names() and \
            self.parameter_workspace.workspace_name == other.parameter_workspace.workspace_name and \
            self.covariance_workspace.workspace_name == other.covariance_workspace.workspace_name and \
            self.tf_asymmetry_fit == other.tf_asymmetry_fit

    def output_workspaces_objects(self) -> list:
        """Returns a list of output workspaces. Do not try place these objects into the ADS as they are not copied."""
        return [output_workspace.workspace for output_workspace in self.output_workspaces]

    def output_workspace_names(self) -> list:
        """Returns a list of the output workspace names."""
        return [output_workspace.workspace_name for output_workspace in self.output_workspaces]

    def output_group_name(self) -> str:
        """Returns the name of the WorkspaceGroup that contains the output workspaces from a fit."""
        _, directory = create_fitted_workspace_name(self.input_workspaces[0], self.fit_function_name)
        return directory[:-1]

    @property
    def parameters(self):
        return self._fit_parameters

    @property
    def parameter_workspace_name(self):
        return self.parameter_workspace.workspace_name

    def add_copy_to_ads(self) -> None:
        """Adds the output workspaces for this fit to the ADS. Must be a copy so the fit history remains a history."""
        workspace_group = WorkspaceGroup()
        add_ws_to_ads(self.output_group_name(), workspace_group)

        for output_workspace in self.output_workspaces:
            add_ws_to_ads(output_workspace.workspace_name, output_workspace.workspace_copy())
            workspace_group.add(output_workspace.workspace_name)

        add_ws_to_ads(self.parameter_workspace.workspace_name, self.parameter_workspace.workspace_copy())
        workspace_group.add(self.parameter_workspace.workspace_name)

        add_ws_to_ads(self.covariance_workspace.workspace_name, self.covariance_workspace.workspace_copy())
        workspace_group.add(self.covariance_workspace.workspace_name)

    def log_names(self, filter_fn=None):
        """
        The names of the logs on the workspaces associated with this fit.

        :filter_fn: An optional unary function to filter the names out. It should accept a log (e.g. a Mantid Kernel
        Property object) and return True if the log should be accepted.
        :return: A list of log names such as ["sample_temp", "sample_magn_field"]
        """
        filter_fn = filter_fn if filter_fn is not None else lambda x: True

        all_names = []
        for output_workspace in self.output_workspaces_objects():
            logs = output_workspace.run().getLogData()
            all_names.extend([log.name for log in logs if filter_fn(log)])

        return all_names

    def has_log(self, log_name):
        """
        :param log_name: A string name
        :return: True if the log exists on all of the input workspaces False, otherwise
        """
        for output_workspace in self.output_workspaces_objects():
            run = output_workspace.run()
            if not run.hasProperty(log_name):
                return False

        return True

    def log_value(self, log_name):
        """
        Compute and return the log value for the named log.
        If the log is a string then the value is converted to a float
        if possible. If the log is a time series then the time-average
        value is computed. If multiple workspaces are part of the fit
        then the values computed above are averaged over each workspace.
        It is assumed that all logs have been checked for existence.
        :param log_name: The name of an existing log
        :return: A single double value
        """

        def value_from_workspace(output_workspace):
            run = output_workspace.run()
            prop = run.getProperty(log_name)
            if hasattr(prop, 'timeAverageValue'):
                return prop.timeAverageValue()
            else:
                try:
                    return float(prop.value)
                except ValueError:
                    return prop.valueAsStr

        values = [value_from_workspace(output_workspace) for output_workspace in self.output_workspaces_objects()]
        try:
            return np.mean(values)
        except TypeError:
            # This will be a string
            if len(values) == 1:
                return values[0]
            elif len(values) > 1:
                return str(values[0]) + " to " + str(values[-1])


class FittingContext(object):
    """Context specific to fitting.

    It holds details are any fits performed including:
       - input workspaces
       - parameters
       - function names
    """

    def __init__(self):
        # A dictionary containing the fit histories for different fitting modes. The key should represent a fitting
        # mode, and the value should be a list of FitInformation's indicating all the fits that have been done up to
        # this point
        self._fit_history: dict = {}

        self.new_fit_results_notifier = Observable()
        self.fit_removed_notifier = Observable()

    def all_latest_fits(self) -> None:
        """Returns the latest fits with unique fit output names for all fitting modes. Override in a child class."""
        raise NotImplementedError("This needs to be overridden by a child class.")

    @property
    def active_fit_history(self) -> None:
        """Returns the fit history for the currently active fitting mode. Override in a child class."""
        raise NotImplementedError("This needs to be overridden by a child class.")

    @active_fit_history.setter
    def active_fit_history(self, fit_history: list) -> None:
        """Sets the fit history for the currently active fitting mode. Override in a child class."""
        raise NotImplementedError("This needs to be overridden by a child class.")

    def clear(self, removed_fits: list = []):
        """Removes all the stored Fits from the context."""
        self.fit_removed_notifier.notify_subscribers(removed_fits)

    def remove_overridden_fits(self) -> None:
        """Removes the fits in the fit history that have been overridden by a newer fit."""
        self.active_fit_history = self._latest_unique_fits_in(self.active_fit_history)

    @staticmethod
    def remove_fit_by_name(fits_history: list, workspace_name: str) -> None:
        """Remove a Fit from the history when an ADS delete event happens on one of its output workspaces."""
        for fit in reversed(fits_history):
            if workspace_name in fit.output_workspace_names() or workspace_name == fit.parameter_workspace_name:
                fits_history.remove(fit)

    def add_fit_from_values(self,
                            input_workspace_names: list,
                            fit_function_name: str,
                            output_workspaces: list,
                            parameter_workspace: StaticWorkspaceWrapper,
                            covariance_workspace: StaticWorkspaceWrapper,
                            global_parameters: list = None,
                            tf_asymmetry_fit: bool = False) -> None:
        """
        Add a new fit information object based on the raw values.
        :param input_workspace_names: A list of input workspace names containing the original data.
        :param fit_function_name: The name of the function used.
        :param output_workspaces: A list of StaticWorkspaceWrapper's containing the output workspaces.
        :param parameter_workspace: A StaticWorkspaceWrapper containing the parameter workspace.
        :param covariance_workspace: A StaticWorkspaceWrapper containing the covariance workspace.
        :param global_parameters: An optional list of global parameters that were tied together during the fit.
        :param tf_asymmetry_fit: An optional flag indicating whether the data is from a TF Asymmetry fit or not.
        """
        self.add_fit(FitInformation(input_workspace_names, fit_function_name, output_workspaces, parameter_workspace,
                                    covariance_workspace, global_parameters, tf_asymmetry_fit))

    def add_fit(self, fit: FitInformation) -> None:
        """
        Add a new fit to the context. Subscribers are notified of the update.
        :param fit: A new FitInformation object
        """
        self.active_fit_history.append(fit)
        self.new_fit_results_notifier.notify_subscribers(fit)

    def fit_function_names(self) -> list:
        """
        :return: a list of unique function names used in the fit
        """
        return list(set([fit.fit_function_name for fit in self.all_latest_fits()]))

    def find_fit_for_input_workspace_list_and_function(self, input_workspace_list, fit_function_name):
        """
        Find the fit in the list whose input workspace matches the input workspace list
        and the specified fit function name
        :param  input_workspace_list: A list of input workspaces
        :param fit_function_name: Fit function name
        :return: A matching fit
        """
        for fit in self.all_latest_fits():
            if fit.input_workspaces == input_workspace_list and fit.fit_function_name == fit_function_name:
                return fit
        else:
            return None

    def log_names(self, filter_fn=None):
        """
        The names of the logs on the workspaces associated with all of the workspaces.

        :filter_fn: An optional unary function to filter the names out. For more information see
        FitInformation.log_names
        :return: A list of names of logs
        """
        return [name for fit in self.all_latest_fits() for name in fit.log_names(filter_fn)]

    def undo_previous_fit(self) -> None:
        """
        Undoes the Fit stored at the top of the fit history. If an identical fit exists further back in the history,
        then its workspaces are loaded into the ADS. Otherwise, the Results tab is notified to remove the fit.
        """
        removed_fit = self.active_fit_history.pop()

        fit_found = self._add_next_identical_fit_to_the_ads(removed_fit)
        if not fit_found:
            self.fit_removed_notifier.notify_subscribers([removed_fit])

    def _add_next_identical_fit_to_the_ads(self, undone_fit: FitInformation) -> bool:
        """
        Adds the next fit in the history that is identical to the undone fit back into the ADS. The history is reversed
        because the end of the list contains the most recent fits.
        :param undone_fit: The Fit that was just undone by the user.
        :returns True if an identical fit was found in the history and added back into the ADS.
        """
        for fit in reversed(self.active_fit_history):
            if fit == undone_fit:
                fit.add_copy_to_ads()
                return True
        return False

    def _latest_unique_fits_in(self, fits_history: list) -> list:
        """Returns a list of fits which all have unique fit output workspaces, and are the most recent of their kind."""
        if len(fits_history) == 0:
            return fits_history

        latest_fits = []
        # Reversed because the fits at the end of the list are the most recently performed fits.
        for fit in reversed(fits_history):
            if fit not in latest_fits:
                latest_fits.append(fit)
        latest_fits.reverse()
        return latest_fits
