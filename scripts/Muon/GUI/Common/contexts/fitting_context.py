# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import OrderedDict
import re

from mantid.api import AnalysisDataService
import numpy as np

from mantidqt.utils.observer_pattern import Observable

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
                 parameter_workspace,
                 fit_function_name,
                 input_workspace,
                 output_workspace_names,
                 global_parameters=None):
        """
        :param parameter_workspace: The workspace wrapper
        that contains all of the parameters from the fit
        :param fit_function_name: The name of the function used
        :param input_workspace: The name or list of names
        of the workspace(s) containing the original data
        :param output_workspace_names: A list containing the names of the output workspaces containing the fits
        :param global_parameters: An optional list of parameters
        that were tied together during the fit
        """
        self._fit_parameters = FitParameters(parameter_workspace,
                                             global_parameters)
        self.fit_function_name = fit_function_name
        self.input_workspaces = [input_workspace] if isinstance(
            input_workspace, str) else input_workspace
        self.output_workspace_names = [output_workspace_names] if isinstance(
            output_workspace_names, str) else output_workspace_names

    def __eq__(self, other):
        """Objects are equal if each member is equal to the other"""
        return self.parameter_workspace_name == other.parameter_workspace_name and \
            self.fit_function_name == other.fit_function_name and \
            self.input_workspaces == other.input_workspaces and \
            self.output_workspace_names == other.output_workspace_names

    @property
    def parameters(self):
        return self._fit_parameters

    @property
    def parameter_workspace_name(self):
        return self._fit_parameters.parameter_workspace_name

    def log_names(self, filter_fn=None):
        """
        The names of the logs on the workspaces
        associated with this fit.

        :filter_fn: An optional unary function to filter the names out. It should accept a log
        and return True if the log should be accepted
        :return: A list of names
        """
        filter_fn = filter_fn if filter_fn is not None else lambda x: True

        all_names = []
        for ws_name in self.output_workspace_names:
            logs = _run(ws_name).getLogData()
            all_names.extend([log.name for log in logs if filter_fn(log)])

        return all_names

    def has_log(self, log_name):
        """
        :param log_name: A string name
        :return: True if the log exists on all of the input workspaces False, otherwise
        """
        for ws_name in self.output_workspace_names:
            run = _run(ws_name)
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
        ads = AnalysisDataService.Instance()

        def value_from_workspace(wksp_name):
            run = ads.retrieve(wksp_name).run()
            prop = run.getProperty(log_name)
            if hasattr(prop, 'timeAverageValue'):
                return prop.timeAverageValue()
            else:
                try:
                    return float(prop.value)
                except ValueError:
                    return prop.valueAsStr

        values = [
            value_from_workspace(wksp_name)
            for wksp_name in self.output_workspace_names
        ]
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

    def __init__(self, fit_list=None):
        self.fit_list = fit_list if fit_list is not None else []
        # Register callbacks with this object to observe when new fits
        # are added
        self.new_fit_results_notifier = Observable()
        self.fit_removed_notifier = Observable()
        self.plot_guess_notifier = Observable()
        self._number_of_fits = 0
        self._number_of_fits_cache = 0
        self._plot_guess = False
        self._guess = None
        self._fit_raw = True
        self._fit_type = "Single"

    def __len__(self):
        """
        :return: The number of fits in the list
        """
        return len(self.fit_list)

    def add_fit_from_values(self,
                            parameter_workspace,
                            fit_function_name,
                            input_workspace,
                            output_workspace_names,
                            global_parameters=None):
        """
        Add a new fit information object based on the raw values.
        See FitInformation constructor for details are arguments.
        """
        self.add_fit(
            FitInformation(parameter_workspace, fit_function_name,
                           input_workspace, output_workspace_names,
                           global_parameters))

    def add_fit(self, fit):
        """
        Add a new fit to the context. Subscribers are notified of the update.
        :param fit: A new FitInformation object
        """
        if fit not in self.fit_list:
            self.fit_list.append(fit)
            self._number_of_fits += 1
        else:
            self.update_fit(fit)

        self.new_fit_results_notifier.notify_subscribers(fit)

    def update_fit(self, updated_fit):
        """
        Updates fit parameters of a fit that is currently stored in the context
        :param updated_fit: A FitInformation object
        """
        for fit in self.fit_list:
            if updated_fit == fit:
                fit._fit_parameters = updated_fit._fit_parameters
                return

    def notify_plot_guess_changed(self, plot_guess, guess_ws):
        # First remove the previous plot_guess from plot
        self.plot_guess = False
        self.plot_guess_notifier.notify_subscribers()

        self.plot_guess = plot_guess
        self.guess_ws = guess_ws
        self.plot_guess_notifier.notify_subscribers()

    def fit_function_names(self):
        """
        :return: a list of unique function names used in the fit
        """
        return list(set([fit.fit_function_name for fit in self.fit_list]))

    def find_output_workspaces_for_input_workspace_name(
            self, input_workspace_name):
        """
        Find the fits in the list whose input workspace matches
        :param input_workspace_name: The name of the input_workspace
        :return: A list of matching fits
        """
        workspace_list = []
        for fit in self.fit_list:
            for index, workspace in enumerate(fit.input_workspaces):
                if workspace == input_workspace_name:
                    workspace_list.append(fit.output_workspace_names[index])

        return workspace_list

    def find_fit_for_input_workspace_list_and_function(
            self, input_workspace_list, fit_function_name):
        """
        Find the fit in the list whose input workspace matches the input workspace list
        and the specified fit function name
        :param  input_workspace_list: A list of input workspaces
        :param fit_function_name: Fit function name
        :return: A matching fit
        """
        for fit in self.fit_list:
            if fit.input_workspaces == input_workspace_list and fit.fit_function_name == fit_function_name:
                return fit
        else:
            return None

    def remove_workspace_by_name(self, workspace_name):
        list_of_fits_to_remove = []
        for fit in self.fit_list:
            if workspace_name in fit.output_workspace_names or workspace_name == fit.parameter_workspace_name:
                self._number_of_fits_cache = 0
                list_of_fits_to_remove.append(fit)

        for fit in list_of_fits_to_remove:
            index = self.fit_list.index(fit)
            if index >= len(self.fit_list) - self._number_of_fits:
                self._number_of_fits -= 1
            self.fit_list.remove(fit)

    def remove_fits_from_stored_fit_list(self, fits):
        removed_fits = []
        for fit in fits:
            if fit in self.fit_list:
                self.fit_list.remove(fit)
                removed_fits += [fit]
                self._number_of_fits -= 1
        self.fit_removed_notifier.notify_subscribers(removed_fits)

    def log_names(self, filter_fn=None):
        """
        The names of the logs on the workspaces associated with all of the workspaces.

        :filter_fn: An optional unary function to filter the names out. For more information see
        FitInformation.log_names
        :return: A list of names of logs
        """
        return [
            name for fit in self.fit_list for name in fit.log_names(filter_fn)
        ]

    def clear(self):
        fits_to_remove = self.fit_list.copy()
        self.remove_fits_from_stored_fit_list(fits_to_remove)

    def remove_all_fits(self):
        removed_fits = self.fit_list
        self.fit_list = []
        self._number_of_fits = 0
        self.fit_removed_notifier.notify_subscribers(removed_fits)

    def remove_latest_fit(self):
        if self.fit_list:
            removed_fit = self.fit_list[-1]
            self.fit_list = self.fit_list[:-1]
            self._number_of_fits -= 1
            self.fit_removed_notifier.notify_subscribers([removed_fit])

    @property
    def number_of_fits(self):
        return self._number_of_fits

    @number_of_fits.setter
    def number_of_fits(self, value):
        self._number_of_fits_cache = self._number_of_fits
        self._number_of_fits = value

    @property
    def fit_type(self):
        return self._fit_type

    @fit_type.setter
    def fit_type(self, fit_type):
        self._fit_type = fit_type

    @property
    def plot_guess(self):
        return self._plot_guess

    @plot_guess.setter
    def plot_guess(self, value):
        self._plot_guess = value

    @property
    def fit_raw(self):
        return self._fit_raw

    @fit_raw.setter
    def fit_raw(self, value):
        self._fit_raw = value

    @property
    def guess_ws(self):
        return self._guess

    @guess_ws.setter
    def guess_ws(self, value):
        self._guess = value


# Private functions
def _run(ws_name):
    """
    :param ws_name: A workspace name in the ADS
    :return: A list of the log data for a workspace
    """
    return AnalysisDataService.Instance().retrieve(ws_name).run()
