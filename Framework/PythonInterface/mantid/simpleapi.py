# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This module defines a function-style API for running Mantid
algorithms. Each algorithm within Mantid is mapped to a Python
function of the same name with the parameters of the algorithm becoming
arguments to the function.

For example:

The Rebin algorithm is mapped to this Python function:

   Rebin(InputWorkspace, OutputWorkspace, Params, PreserveEvents=None, Version=1)

It returns the output workspace and this workspace has the same name as
the variable it is assigned to, i.e.

   rebinned = Rebin(input, Params = '0.1,0.05,10')

would call Rebin with the given parameters and create a workspace called 'rebinned'
and assign it to the rebinned variable.

Importing this module starts the FrameworkManager instance.
"""

# std libs
from collections import OrderedDict, namedtuple
from contextlib import contextmanager
import datetime
from dateutil.parser import parse as parse_date
import os
import sys

import mantid

# This is a simple API so give access to the aliases by default as well
from mantid import api as _api, kernel as _kernel
from mantid import apiVersion  # noqa: F401
from mantid.kernel import plugins as _plugin_helper
from mantid.kernel import ConfigService, logger
from mantid.kernel.funcinspect import (
    customise_func as _customise_func,
    lhs_info as _lhs_info,
    replace_signature as _replace_signature,
    LazyFunctionSignature,
)

# register matplotlib projection
try:
    from mantid import plots  # noqa
    from mantid.plots._compatability import plotSpectrum, plotBin  # noqa
except ImportError:
    pass  # matplotlib is unavailable

from mantid.kernel._aliases import *
from mantid.api._aliases import *
from mantid.fitfunctions import *

MODULE_NAME = "simpleapi"

# List of specialized algorithms
__SPECIALIZED_FUNCTIONS__ = ["Load", "StartLiveData", "CutMD", "RenameWorkspace"]
# List of specialized algorithms
__MDCOORD_FUNCTIONS__ = ["PeakIntensityVsRadius", "CentroidPeaksMD", "IntegratePeaksMD"]
# The "magic" keyword to enable/disable logging
__LOGGING_KEYWORD__ = "EnableLogging"
# The "magic" keyword to run as a child algorithm explicitly without storing on ADS
__STORE_KEYWORD__ = "StoreInADS"
# This is the default value for __STORE_KEYWORD__
__STORE_ADS_DEFAULT__ = True


def specialization_exists(name):
    """
    Returns true if a specialization for the given name
    already exists, false otherwise

    :param name: The name of a possible new function
    """
    return name in __SPECIALIZED_FUNCTIONS__


def extract_progress_kwargs(kwargs):
    """
    Returns tuple(startProgress, endProgress, kwargs) with the special
    keywords removed from kwargs. If the progress keywords are not
    specified, None will be returned in their place.
    """
    start = kwargs.pop("startProgress", None)
    end = kwargs.pop("endProgress", None)
    return start, end, kwargs


def Load(*args, **kwargs):
    """
    Load is a more flexible algorithm than other Mantid algorithms.
    It's aim is to discover the correct loading algorithm for a
    given file. This flexibility comes at the expense of knowing the
    properties out right before the file is specified.

    The argument list for the Load function has to be more flexible to
    allow this searching to occur. Two arguments must be specified:

      - Filename :: The name of the file,
      - OutputWorkspace :: The name of the workspace,

    either as the first two arguments in the list or as keywords. Any other
    properties that the Load algorithm has can be specified by keyword only.

    Some common keywords are:
     - SpectrumMin,
     - SpectrumMax,
     - SpectrumList,
     - EntryNumber

    Example:
      # Simple usage, ISIS NeXus file
      run_ws = Load('INSTR00001000.nxs')

      # Histogram NeXus with SpectrumMin and SpectrumMax = 1
      run_ws = Load('INSTR00001000.nxs', SpectrumMin=1,SpectrumMax=1)

      # Event NeXus with precount on
      event_ws = Load('INSTR_1000_event.nxs', Precount=True)

      # The output workspace name is picked up from the LHS unless overridden
      Load('INSTR00001000.nxs',OutputWorkspace='run_ws')
    """
    (filename,) = _get_mandatory_args("Load", ["Filename"], *args, **kwargs)
    if not filename:
        # If we try to set property with a None type we get a unhelpful error about allocators
        # so check up front here
        raise ValueError(
            "Problem with supplied Filename. The value given was a 'None' "
            "type and cannot be used. Please ensure the Filename is set"
            " to the path of the file."
        )

    # Create and execute
    (_startProgress, _endProgress, kwargs) = extract_progress_kwargs(kwargs)
    algm = _create_algorithm_object("Load", startProgress=_startProgress, endProgress=_endProgress)
    _set_logging_option(algm, kwargs)
    _set_store_ads(algm, kwargs)
    try:
        algm.setProperty("Filename", filename)  # Must be set first
    except ValueError as ve:
        msg = f'Problem setting "Filename" in Load: {ve}'
        raise ValueError(
            msg + "\nIf the file has been found "
            "but you got this error, you might not have read permissions "
            "or the file might be corrupted.\nIf the file has not been found, "
            "you might have forgotten to add its location in the data search "
            "directories."
        )
    # Remove from keywords so it is not set twice
    if "Filename" in kwargs:
        del kwargs["Filename"]
    lhs = _kernel.funcinspect.lhs_info()
    # If the output has not been assigned to anything, i.e. lhs[0] = 0 and kwargs does not have OutputWorkspace
    # then raise a more helpful error than what we would get from an algorithm
    if lhs[0] == 0 and "OutputWorkspace" not in kwargs:
        raise RuntimeError(
            "Unable to set output workspace name. Please either assign the output of "
            "Load to a variable or use the OutputWorkspace keyword."
        )

    lhs_args = _get_args_from_lhs(lhs, algm)
    final_keywords = _merge_keywords_with_lhs(kwargs, lhs_args)
    # Check for any properties that aren't known and warn they will not be used
    for key in list(final_keywords.keys()):
        if key not in algm:
            logger.warning("You've passed a property (%s) to Load() that doesn't apply to this file type." % key)
            del final_keywords[key]
    set_properties(algm, **final_keywords)
    algm.execute()

    # If a WorkspaceGroup was loaded then there will be a set of properties that have an underscore in the name
    # and users will simply expect the groups to be returned NOT the groups + workspaces.
    return _gather_returns("Load", lhs, algm, ignore_regex=["LoaderName", "LoaderVersion", ".*_.*"])


######################################################################


def StartLiveData(*args, **kwargs):
    """
    StartLiveData dynamically adds the properties of the specific LiveListener
    that is used to itself, to allow usage such as the following:

        StartLiveData(Instrument='ISIS_Histogram', ...
                      PeriodList=[1,3], SpectraList=[2,4,6])

    Where PeriodList and SpectraList are properties of the ISISHistoDataListener
    rather than of StartLiveData. For StartLiveData to know those are valid
    properties, however, it first needs to know what the Instrument is.

    This is a similar situation as in the Load algorithm, where the Filename
    must be provided before other properties become available, and so it is
    solved here in the same way.
    """
    (instrument,) = _get_mandatory_args("StartLiveData", ["Instrument"], *args, **kwargs)

    # Create and execute
    (_startProgress, _endProgress, kwargs) = extract_progress_kwargs(kwargs)
    algm = _create_algorithm_object("StartLiveData", startProgress=_startProgress, endProgress=_endProgress)
    _set_logging_option(algm, kwargs)
    _set_store_ads(algm, kwargs)

    # Some properties have side effects and must be set separately
    def handleSpecialProperty(name, value=None):
        try:
            if value is None:
                value = kwargs.pop(name)
            else:
                # We don't need the value, but still need to remove from kwargs
                # so that this property isn't set again later
                kwargs.pop(name, None)
            algm.setProperty(name, value)

        except ValueError as ve:
            raise ValueError('Problem setting "{}" in {}-v{}: {}'.format(name, algm.name(), algm.version(), str(ve)))
        except KeyError:
            pass  # ignore if kwargs[name] doesn't exist

    # Listener properties depend on these values, so they must be set first
    handleSpecialProperty("Instrument", instrument)
    handleSpecialProperty("Connection")
    handleSpecialProperty("Listener")

    # LHS Handling currently unsupported for StartLiveData
    lhs = _kernel.funcinspect.lhs_info()
    if lhs[0] > 0:  # Number of terms on the lhs
        raise RuntimeError(
            "Assigning the output of StartLiveData is currently "
            "unsupported due to limitations of the simpleapi. "
            "Please call StartLiveData without assigning it to "
            "to anything."
        )

    lhs_args = _get_args_from_lhs(lhs, algm)
    final_keywords = _merge_keywords_with_lhs(kwargs, lhs_args)

    # Check for any properties that aren't known and warn they will not be used
    for key in list(final_keywords.keys()):
        if key not in algm:
            logger.warning("You've passed a property (%s) to StartLiveData() " "that doesn't apply to this Instrument." % key)
            del final_keywords[key]

    set_properties(algm, **final_keywords)
    algm.execute()

    return _gather_returns("StartLiveData", lhs, algm)


# ---------------------------- Fit ---------------------------------------------


def fitting_algorithm(inout=False):
    """
    Decorator generating code for fitting algorithms (Fit, CalculateChiSquared,
    EvaluateFunction).
    When applied to a function definition this decorator replaces its code
    with code of function 'wrapper' defined below.
    :param inout: if True, return also the InOut properties of algorithm f
    """

    def inner_fitting_algorithm(f):
        """
        :param f: algorithm calling Fit
        """

        def wrapper(*args, **kwargs):
            function, input_workspace = _get_mandatory_args(function_name, ["Function", "InputWorkspace"], *args, **kwargs)
            # Remove from keywords so it is not set twice
            if "Function" in kwargs:
                del kwargs["Function"]
            if "InputWorkspace" in kwargs:
                del kwargs["InputWorkspace"]

            # Check for behaviour consistent with old API
            if isinstance(function, str) and function in _api.AnalysisDataService:
                msg = "Fit API has changed. The function must now come " + "first in the argument list and the workspace second."
                raise ValueError(msg)
            # Deal with case where function is a FunctionWrapper.
            if isinstance(function, FunctionWrapper):
                function = function.__str__()

            # Create and execute
            algm = _create_algorithm_object(function_name)
            _set_logging_option(algm, kwargs)
            _set_store_ads(algm, kwargs)
            if "EvaluationType" in kwargs:
                algm.setProperty("EvaluationType", kwargs["EvaluationType"])
                del kwargs["EvaluationType"]
            algm.setProperty("Function", function)  # Must be set first
            if input_workspace is not None:
                algm.setProperty("InputWorkspace", input_workspace)
            else:
                del algm["InputWorkspace"]

            # Set all workspace properties before others
            for key in list(kwargs.keys()):
                if key.startswith("InputWorkspace_"):
                    algm.setProperty(key, kwargs[key])
                    del kwargs[key]

            lhs = _lhs_info()
            # Check for unknown properties and warn they will not be used
            for key in list(kwargs.keys()):
                if key not in algm:
                    msg = "Property {} to {} does not apply to any of the input workspaces".format(key, function_name)
                    logger.warning(msg)
                    del kwargs[key]
            set_properties(algm, **kwargs)
            algm.execute()
            return _gather_returns(function_name, lhs, algm, inout=inout)

        # end
        function_name = f.__name__
        signature = ("\bFunction, InputWorkspace", "**kwargs")
        fwrapper = _customise_func(wrapper, function_name, signature, f.__doc__)
        if function_name not in __SPECIALIZED_FUNCTIONS__:
            __SPECIALIZED_FUNCTIONS__.append(function_name)
        return fwrapper

    return inner_fitting_algorithm


# Use a python decorator (defined above) to generate the code for this function.
@fitting_algorithm(inout=True)
def Fit(*args, **kwargs):
    """
    Fit defines the interface to the fitting within Mantid.
    It can work with arbitrary data sources and therefore some options
    are only available when the function & workspace type are known.

    This simple wrapper takes the Function (as a string or a
    FunctionWrapper object) and the InputWorkspace
    as the first two arguments. The remaining arguments must be
    specified by keyword.

    Example:
      Fit(Function='name=LinearBackground,A0=0.3', InputWorkspace=dataWS',
          StartX='0.05',EndX='1.0',Output="Z1")
    """
    return None


# Use a python decorator (defined above) to generate the code for this function.
@fitting_algorithm()
def CalculateChiSquared(*args, **kwargs):
    """
    This function calculates chi squared calculation for a function and a data set.
    The data set is defined in a way similar to Fit algorithm.

    Example:
      chi2_1, chi2_2, chi2_3, chi2_4 = \\
        CalculateChiSquared(Function='name=LinearBackground,A0=0.3', InputWorkspace=dataWS',
            StartX='0.05',EndX='1.0')
    """
    return None


# Use a python decorator (defined above) to generate the code for this function.
@fitting_algorithm()
def EvaluateFunction(*args, **kwargs):
    """
    This function evaluates a function on a data set.
    The data set is defined in a way similar to Fit algorithm.

    Example:
      EvaluateFunction(Function='name=LinearBackground,A0=0.3', InputWorkspace=dataWS',
          StartX='0.05',EndX='1.0',Output="Z1")
    """
    return None


# Use a python decorator (defined above) to generate the code for this function.
@fitting_algorithm()
def QENSFitSimultaneous(*args, **kwargs):
    """
    QENSFitSimultaneous is used to fit QENS data
    The data set is defined in a way similar to Fit algorithm.

    Example:
      QENSFitSimultaneous(Function='name=LinearBackground,A0=0.3', InputWorkspace=dataWS',
                          StartX='0.05',EndX='1.0',Output="Z1")
    """
    return None


# Use a python decorator (defined above) to generate the code for this function.
@fitting_algorithm()
def ConvolutionFitSimultaneous(*args, **kwargs):
    """
    ConvolutionFitSimultaneous is used to fit QENS convolution data
    The data set is defined in a way similar to Fit algorithm.
    """
    return None


# Use a python decorator (defined above) to generate the code for this function.
@fitting_algorithm()
def IqtFitSimultaneous(*args, **kwargs):
    """
    IqtFitSimultaneous is used to fit I(Q,t) data
    The data set is defined in a way similar to Fit algorithm.
    """
    return None


# --------------------------------------------------- --------------------------


def CutMD(*args, **kwargs):  # noqa: C901
    """
    Slices multidimensional workspaces using input projection information and binning limits.
    """
    (in_wss,) = _get_mandatory_args("CutMD", ["InputWorkspace"], *args, **kwargs)

    # If the input isn't a list, wrap it in one so we can iterate easily
    if isinstance(in_wss, list):
        in_list = in_wss
        handling_multiple_workspaces = True
    else:
        in_list = [in_wss]
        handling_multiple_workspaces = False

    # Remove from keywords so it is not set twice
    if "InputWorkspace" in kwargs:
        del kwargs["InputWorkspace"]

    # Make sure we were given some output workspace names
    lhs = _lhs_info()
    if lhs[0] == 0 and "OutputWorkspace" not in kwargs:
        raise RuntimeError(
            "Unable to set output workspace name. Please either assign the output of "
            "CutMD to a variable or use the OutputWorkspace keyword."
        )

    # Take what we were given
    if "OutputWorkspace" in kwargs:
        out_names = kwargs["OutputWorkspace"]
    else:
        out_names = list(lhs[1])

    # Ensure the output names we were given are valid
    if handling_multiple_workspaces:
        if not isinstance(out_names, list):
            raise RuntimeError("Multiple OutputWorkspaces must be given as a list when" " processing multiple InputWorkspaces.")
    else:
        # We wrap in a list for our convenience. The user must not pass us one though.
        if not isinstance(out_names, list):
            out_names = [out_names]
        elif len(out_names) != 1:
            raise RuntimeError("Only one OutputWorkspace required")

    if len(out_names) != len(in_list):
        raise RuntimeError("Different number of input and output workspaces given.")

    # Split PBins up into P1Bin, P2Bin, etc.
    if "PBins" in kwargs:
        bins = kwargs["PBins"]
        del kwargs["PBins"]
        if isinstance(bins, tuple) or isinstance(bins, list):
            for bin_index in range(len(bins)):
                kwargs["P{0}Bin".format(bin_index + 1)] = bins[bin_index]

    # Create and execute
    (_startProgress, _endProgress, kwargs) = extract_progress_kwargs(kwargs)
    algm = _create_algorithm_object("CutMD", startProgress=_startProgress, endProgress=_endProgress)
    _set_logging_option(algm, kwargs)
    _set_store_ads(algm, kwargs)

    # Now check that all the kwargs we've got are correct
    for key in kwargs.keys():
        if key not in algm:
            msg = f"'{key}' is an invalid keyword argument for {algm.name()}-v{algm.version()}"
            raise TypeError(msg)

    # We're now going to build to_process, which is the list of workspaces we want to process.
    to_process = list()
    for i in range(len(in_list)):
        ws = in_list[i]

        if isinstance(ws, _api.Workspace):
            # It's a workspace, do nothing to it
            to_process.append(ws)
        elif isinstance(ws, str):
            if ws in mtd:
                # It's a name of something in the ads, just take it from the ads
                to_process.append(_api.AnalysisDataService[ws])
            else:
                # Let's try treating it as a filename
                load_alg = AlgorithmManager.create("Load")
                load_alg.setLogging(True)
                load_alg.setAlwaysStoreInADS(False)
                load_alg.setProperty("Filename", ws)
                load_alg.setProperty("OutputWorkspace", "__loaded_by_cutmd_{0}".format(i + 1))
                load_alg.execute()
                if not load_alg.isExecuted():
                    raise TypeError("Failed to load " + ws)
                wsn = load_alg.getProperty("OutputWorkspace").valueAsStr
                to_process.append(_api.AnalysisDataService[wsn])
        else:
            raise TypeError("Unexpected type: " + type(ws))

    # Run the algorithm across the inputs and outputs
    for i in range(len(to_process)):
        set_properties(algm, **kwargs)
        algm.setProperty("InputWorkspace", to_process[i])
        algm.setProperty("OutputWorkspace", out_names[i])
        algm.execute()

    # Get the workspace objects so we can return them
    for i in range(len(out_names)):
        out_names[i] = _api.AnalysisDataService[out_names[i]]

    # We should only return a list if we're handling multiple workspaces
    if handling_multiple_workspaces:
        return out_names
    else:
        return out_names[0]


_replace_signature(CutMD, ("\bInputWorkspace", "**kwargs"))


def RenameWorkspace(*args, **kwargs):
    """Rename workspace with option to renaming monitors
    workspace attached to current workspace.
    """
    arguments = {}
    lhs = _kernel.funcinspect.lhs_info()

    # convert positional args to keyword arguments
    if lhs[0] > 0 and "OutputWorkspace" not in kwargs:
        arguments["OutputWorkspace"] = lhs[1][0]
        for name, value in zip(("InputWorkspace", "RenameMonitors"), args):
            arguments[name] = value
    else:
        for name, value in zip(("InputWorkspace", "OutputWorkspace", "RenameMonitors"), args):
            arguments[name] = value

    arguments.update(kwargs)

    if "OutputWorkspace" not in arguments:
        raise RuntimeError(
            "Unable to set output workspace name."
            " Please either assign the output of "
            "RenameWorkspace to a variable or use the OutputWorkspace keyword."
        )

    # Create and execute
    (_startProgress, _endProgress, kwargs) = extract_progress_kwargs(kwargs)
    algm = _create_algorithm_object("RenameWorkspace", startProgress=_startProgress, endProgress=_endProgress)
    _set_logging_option(algm, arguments)
    algm.setAlwaysStoreInADS(True)
    # does not make sense otherwise, this overwrites even the __STORE_ADS_DEFAULT__
    if __STORE_KEYWORD__ in arguments and arguments[__STORE_KEYWORD__] is not True:
        raise KeyError("RenameWorkspace operates only on named workspaces in ADS.")

    for key, val in arguments.items():
        algm.setProperty(key, val)

    algm.execute()

    return _gather_returns("RenameWorkspace", lhs, algm)


_replace_signature(RenameWorkspace, ("\bInputWorkspace,[OutputWorkspace],[True||False]", "**kwargs"))


def _get_function_spec(func):
    """Get the python function signature for the given function object

    :param func: A Python function object
    """
    import inspect

    try:
        argspec = inspect.getfullargspec(func)
    except TypeError:
        return ""
    # Algorithm functions have varargs set not args
    args = argspec[0]
    if args:
        # For methods strip the self argument
        if hasattr(func, "im_func"):
            args = args[1:]
        defs = argspec[3]
    elif argspec[1] is not None:
        # Get from varargs/keywords
        arg_str = argspec[1].strip().lstrip("\b")
        defs = []
        # Keyword args
        kwargs = argspec[2]
        if kwargs is not None:
            kwargs = kwargs.strip().lstrip("\b\b")
            if kwargs == "kwargs":
                kwargs = "**" + kwargs + "=None"
            arg_str += ",%s" % kwargs
        # Any default argument appears in the string
        # on the rhs of an equal
        for arg in arg_str.split(","):
            arg = arg.strip()
            if "=" in arg:
                arg_token = arg.split("=")
                args.append(arg_token[0])
                defs.append(arg_token[1])
            else:
                args.append(arg)
        if len(defs) == 0:
            defs = None
    else:
        return ""

    if defs is None:
        calltip = ",".join(args)
        calltip = "(" + calltip + ")"
    else:
        # The defaults list contains the default values for the last n arguments
        diff = len(args) - len(defs)
        calltip = ""
        for index in range(len(args) - 1, -1, -1):
            def_index = index - diff
            if def_index >= 0:
                calltip = "[" + args[index] + "]," + calltip
            else:
                calltip = args[index] + "," + calltip
        calltip = "(" + calltip.rstrip(",") + ")"
    return calltip


def _get_mandatory_args(func_name, required_args, *args, **kwargs):
    """Given a list of required arguments, parse them
    from the given args & kwargs and raise an error if they
    are not provided

    :param func_name: The name of the function call
    :type str.
    :param required_args: A list of names of required arguments
    :type list.
    :param args :: The positional arguments to check
    :type dict.
    :param kwargs :: The keyword arguments to check
    :type dict.
    :returns: A tuple of provided mandatory arguments
    """

    def get_argument_value(key, dict_containing_key):
        try:
            val = dict_containing_key[key]
            return val
        except KeyError:
            # raise TypeError similar to python
            raise TypeError(f"{func_name} missing required argument: '{key}'")

    nrequired = len(required_args)
    npositional = len(args)

    if npositional == 0:
        mandatory_args = []
        for arg in required_args:
            mandatory_args.append(get_argument_value(arg, kwargs))
    elif npositional == nrequired:
        mandatory_args = args
    elif npositional < nrequired:
        mandatory_args = []
        for value in args:
            mandatory_args.append(value)
        # Get rest from keywords
        for arg in required_args[npositional:]:
            mandatory_args.append(get_argument_value(arg, kwargs))
    else:
        reqd_as_str = ",".join(required_args).strip(",")
        raise RuntimeError(
            '%s() takes "%s" as positional arguments. Other arguments must be specified by keyword.' % (func_name, reqd_as_str)
        )
    return tuple(mandatory_args)


def _check_mandatory_args(algorithm, _algm_object, error, *args, **kwargs):
    """When a runtime error of the form 'Some invalid Properties found'
    is thrown call this function to return more specific message to user in
    the python output.
    """
    missing_arg_list = []
    # Returns all user defined properties
    props = _algm_object.mandatoryProperties()
    # Add given positional arguments to keyword arguments
    for key, arg in zip(props[: len(args)], args):
        kwargs[key] = arg
    for p in props:
        prop = _algm_object.getProperty(p)
        # Mandatory properties are ones with invalid defaults
        if isinstance(prop.isValid, str):
            valid_str = prop.isValid
        else:
            valid_str = prop.isValid()
        if len(valid_str) > 0 and p not in kwargs.keys():
            missing_arg_list.append(p)
    if len(missing_arg_list) != 0:
        # raise TypeError similar to python
        missing_arg_list = [f"'{arg}'" for arg in missing_arg_list]
        raise TypeError(f"{algorithm} missing required arguments: " + ", ".join(missing_arg_list))
    # If the error was not caused by missing property the algorithm specific error should suffice
    else:
        msg = f"in running {algorithm}: {str(error)}"
        if "Some invalid Properties found" in str(error):
            raise TypeError(msg)
        else:
            raise RuntimeError(msg)


# ------------------------ General simple function calls ----------------------
def _is_workspace_property(prop):
    """
    Returns true if the property is a workspace property.

    Currently several properties , i.e WorspaceProperty<EventWorkspace>
    cannot be recognised by Python so we have to resort to a name test

    :param prop: A property object
    :type Property
    :returns: True if the property is considered to be of type workspace
    """
    if isinstance(prop, _api.IWorkspaceProperty):
        return True
    if type(prop) is _kernel.Property and "Workspace" in prop.name:
        return True
    else:
        # Doesn't look like a workspace property
        return False


def _is_function_property(prop):
    """
    Returns True if the property is a fit function

    :param prop: A property object
    :type Property
    :return:  True if the property is considered a fit function
    """
    return isinstance(prop, _api.FunctionProperty)


def _get_args_from_lhs(lhs, algm_obj):
    """
    Return the extra arguments that are to be passed to the algorithm
    from the information in the lhs tuple. These are basically the names
    of output workspaces.
    The algorithm properties are iterated over in the same order
    they were created within the wrapper and for each output
    workspace property an entry is added to the returned dictionary
    that contains {PropertyName:lhs_name}.

    :param lhs: A 2-tuple that contains the number of variables supplied on the lhs of the
    function call and the names of these variables
    :param algm_obj: An initialised algorithm object
    :returns: A dictionary mapping property names to the values extracted from the lhs variables
    """

    ret_names = lhs[1]
    extra_args = {}

    output_props = [algm_obj.getProperty(p) for p in algm_obj.outputProperties()]

    nprops = len(output_props)
    nnames = len(ret_names)

    name = 0

    for p in output_props:
        if _is_workspace_property(p):
            # Check nnames is greater than 0 and less than nprops
            if 0 < nnames < nprops:
                extra_args[p.name] = ret_names[0]  # match argument to property name
                ret_names = ret_names[1:]
                nnames -= 1
            elif nnames > 0:
                extra_args[p.name] = ret_names[name]

        name += 1

    return extra_args


def _merge_keywords_with_lhs(keywords, lhs_args):
    """
    Merges the arguments from the two dictionaries specified
    by the keywords passed to a function and the lhs arguments
    that have been parsed. Any value in keywords overrides on
    in lhs_args.

    :param keywords: A dictionary of keywords that has been passed to the function call
    :param lhs_args: A dictionary of arguments retrieved from the lhs of the function call
    """
    final_keywords = lhs_args
    final_keywords.update(keywords)
    return final_keywords


def _gather_returns(func_name, lhs, algm_obj, ignore_regex=None, inout=False):
    """Gather the return values and ensure they are in the
    correct order as defined by the output properties and
    return them as a tuple. If their is a single return
    value it is returned on its own

    :param func_name: The name of the calling function.
    :param lhs: A 2-tuple that contains the number of variables supplied on the
    lhs of the function call and the names of these variables.
    :param algm_obj: An executed algorithm object.
    :param ignore_regex: A list of strings containing regex expressions to match
    :param inout : gather also the InOut properties if True.
    against property names that will be ignored & not returned.
    """
    if ignore_regex is None:
        ignore_regex = []

    import re

    def ignore_property(name_to_check, regex_to_ignore):
        for regex in regex_to_ignore:
            if regex.match(name_to_check) is not None:
                return True
        # Matched nothing
        return False

    if isinstance(ignore_regex, str):
        ignore_regex = [ignore_regex]
    # Compile regexes
    for index, expr in enumerate(ignore_regex):
        ignore_regex[index] = re.compile(expr)

    retvals = OrderedDict()
    names = algm_obj.outputProperties()
    if inout:
        names.extend(algm_obj.inoutProperties())
    for name in names:
        if ignore_property(name, ignore_regex):
            continue
        prop = algm_obj.getProperty(name)

        if _is_workspace_property(prop):
            value = None
            if hasattr(prop, "value"):
                value = prop.value
            if value is not None:
                retvals[name] = value
            else:
                try:
                    value_str = prop.valueAsStr
                    retvals[name] = _api.AnalysisDataService[value_str]
                except KeyError:
                    if not (hasattr(prop, "isOptional") and prop.isOptional()) and prop.direction == _kernel.Direction.InOut:
                        raise RuntimeError(
                            "Mandatory InOut workspace property '%s' on "
                            "algorithm '%s' has not been set correctly. " % (name, algm_obj.name())
                        )
        elif _is_function_property(prop):
            retvals[name] = FunctionWrapper.wrap(prop.value)
        else:
            if hasattr(prop, "value"):
                retvals[name] = prop.value
            else:
                raise RuntimeError(
                    'Internal error. Unknown property type encountered. "%s" '
                    'on algorithm "%s" is not understood by '
                    "Python. Please contact development team" % (name, algm_obj.name())
                )

    # If there is a snippet of code as follows
    # foo, bar, baz = simpleAPI.myFunc(...)
    # The number of values on LHS is 3 (foo, bar baz) and the number of
    # returned values is the number of values myFunc(...) returns
    number_of_returned_values = len(retvals)
    number_of_values_on_lhs = lhs[0]

    # If we have more than one value but not the same number of values throw
    if number_of_values_on_lhs > 1 and number_of_returned_values != number_of_values_on_lhs:
        # There is a discrepancy in the number are unpacking variables
        # Let's not have the more cryptic unpacking error raised
        raise RuntimeError(
            "%s is trying to return %d output(s) but you have provided %d variable(s). "
            "These numbers must match." % (func_name, number_of_returned_values, number_of_values_on_lhs)
        )
    if number_of_returned_values > 0:
        ret_type = namedtuple(func_name + "_returns", retvals.keys())
        ret_value = ret_type(**retvals)
        if number_of_returned_values == 1:
            return ret_value[0]
        else:
            return ret_value
    else:
        return None


def _set_logging_option(algm_obj, kwargs):
    """
    Checks the keyword arguments for the _LOGGING keyword, sets the state of the
    algorithm logging accordingly and removes the value from the dictionary. If the keyword
    does not exist then it does nothing.

    :param algm_obj: An initialised algorithm object
    :param **kwargs: A dictionary of the keyword arguments passed to the simple function call
    """
    import inspect

    parent = _find_parent_pythonalgorithm(inspect.currentframe())
    logging_default = parent.isLogging() if parent is not None else True
    algm_obj.setLogging(kwargs.pop(__LOGGING_KEYWORD__, logging_default))


def _set_store_ads(algm_obj, kwargs):
    """
    Sets to always store in ADS, unless StoreInADS=False

    :param algm_obj: An initialised algorithm object
    :param **kwargs: A dictionary of the keyword arguments passed to the simple function call
    """
    algm_obj.setAlwaysStoreInADS(kwargs.pop(__STORE_KEYWORD__, __STORE_ADS_DEFAULT__))


def set_properties(alg_object, *args, **kwargs):
    """
    Set all of the properties of the algorithm. There is no guarantee of
    the order the properties will be set
    :param alg_object: An initialised algorithm object
    :param args: Positional arguments
    :param kwargs: Keyword arguments
    """

    def do_set_property(name, new_value):
        if new_value is None:
            return
        try:
            if isinstance(new_value, _kernel.DataItem) and new_value.name():
                alg_object.setPropertyValue(key, new_value.name())
            else:
                alg_object.setProperty(key, new_value)
        except (RuntimeError, TypeError, ValueError) as e:
            if str(e).startswith("Unknown property search object") or "is an invalid keyword argument for" in str(e):
                # raise TypeError similar to python
                msg = f"'{name}' is an invalid keyword argument for {alg_object.name()}-v{alg_object.version()}"
                raise TypeError(msg)
            else:
                # re-raise the error with clearer message
                msg = 'Problem setting "{}" in {}-v{}: {}'.format(name, alg_object.name(), alg_object.version(), str(e))
                raise e.__class__(msg) from e

    # end
    if len(args) > 0:
        mandatory_props = alg_object.mandatoryProperties()
    else:
        mandatory_props = []

    postponed = []
    for key, value in kwargs.items():
        if key in mandatory_props:
            mandatory_props.remove(key)
        if "IndexSet" in key:
            # The `IndexSet` sub-property of the "workspace property with index"
            # must be set after the workspace since it is validated based on in.
            postponed.append((key, value))
            continue
        do_set_property(key, value)
    for key, value in postponed:
        do_set_property(key, value)

    # zip stops at the length of the shorter list
    for key, value in zip(mandatory_props, args):
        do_set_property(key, value)


def _create_algorithm_function(name, version, algm_object):  # noqa: C901
    """
    Create a function that will set up and execute an algorithm.
    The help that will be displayed is that of the most recent version.
    :param name: name of the algorithm
    :param version: The version of the algorithm
    :param algm_object: the created algorithm object.
    """

    def algorithm_wrapper(alias=None):  # noqa: C901
        r"""
        @brief Creates a wrapper object around the algorithm functions.
        @param str alias: Non-empty when the algorithm is to be invoked with this alias instead of its name.
                          Default `None` indicates an alias is not being used.
        """

        class Wrapper:
            __slots__ = ["__name__", "__signature__", "_alias"]

            @staticmethod
            def _init_alias(algm_alias):
                r"""
                @brief Encapsulate alias features on a namedtuple
                @param str algm_alias
                """
                deprecated = algm_object.aliasDeprecated()  # non-empty string when alias set to be deprecated
                if deprecated:
                    try:
                        parse_date(deprecated)
                    except ValueError:
                        deprecated = ""
                        logger.error(f"Alias deprecation date {deprecated} must be in ISO8601 format")
                AlgorithmAlias = namedtuple("AlgorithmAlias", "name, deprecated")
                return AlgorithmAlias(algm_alias, deprecated)

            def __init__(self, algm_alias=None):
                self._alias = self._init_alias(algm_alias) if algm_alias else None

            def __getattribute__(self, item):
                obj = object.__getattribute__(self, item)
                if obj is None and item == "__doc__":  # Set doc if accessed directly
                    obj = object.__getattribute__(self, "__class__")
                    algm_object.initialize()
                    setattr(obj, "__doc__", algm_object.docString())
                    return obj.__doc__
                if item == "__class__" and obj.__doc__ is None:  # Set doc if class is accessed.
                    algm_object.initialize()
                    setattr(obj, "__doc__", algm_object.docString())
                return obj

            def __call__(self, *args, **kwargs):
                """
                Note that if the Version parameter is passed, we will create
                the proper version of the algorithm without failing.

                If both startProgress and endProgress are supplied they will
                be used.
                """

                # Check at runtime whether to throw upon alias deprecation.
                if self._alias and self._alias.deprecated:
                    deprecated = parse_date(self._alias.deprecated) < datetime.datetime.today()
                    deprecated_action = ConfigService.Instance().get("algorithms.alias.deprecated", "Log").lower()
                    if deprecated and deprecated_action == "raise":
                        raise RuntimeError(f"Use of algorithm alias {self._alias.name} not allowed. Use {name} instead")

                _version = version
                if "Version" in kwargs:
                    _version = kwargs["Version"]
                    del kwargs["Version"]

                _startProgress, _endProgress = (None, None)
                if "startProgress" in kwargs:
                    _startProgress = kwargs["startProgress"]
                    del kwargs["startProgress"]
                if "endProgress" in kwargs:
                    _endProgress = kwargs["endProgress"]
                    del kwargs["endProgress"]

                algm = _create_algorithm_object(name, _version, _startProgress, _endProgress)
                _set_logging_option(algm, kwargs)
                _set_store_ads(algm, kwargs)

                # Temporary removal of unneeded parameter from user's python scripts
                if "CoordinatesToUse" in kwargs and name in __MDCOORD_FUNCTIONS__:
                    del kwargs["CoordinatesToUse"]

                # a change in parameters should get a better error message
                if algm.name() in ["LoadEventNexus", "LoadNexusMonitors"]:
                    for propname in ["MonitorsAsEvents", "LoadEventMonitors", "LoadHistoMonitors"]:
                        if propname in kwargs:
                            suggest = "LoadOnly"
                            if algm.name() == "LoadEventNexus":
                                suggest = "MonitorsLoadOnly"
                            msg = 'Deprecated property "{}" in {}. Use "{}" instead'.format(propname, algm.name(), suggest)
                            raise ValueError(msg)

                frame = kwargs.pop("__LHS_FRAME_OBJECT__", None)

                lhs = _kernel.funcinspect.lhs_info(frame=frame)
                lhs_args = _get_args_from_lhs(lhs, algm)
                final_keywords = _merge_keywords_with_lhs(kwargs, lhs_args)
                set_properties(algm, *args, **final_keywords)
                try:
                    algm.execute()
                except RuntimeError as e:
                    if e.args[0] == "Some invalid Properties found":
                        # Check for missing mandatory parameters
                        _check_mandatory_args(name, algm, e, *args, **kwargs)
                    else:
                        msg = "{}-v{}: {}".format(algm.name(), algm.version(), str(e))
                        raise RuntimeError(msg) from e

                if self._alias and self._alias.deprecated:
                    logger.error(f"Algorithm alias {self._alias.name} is deprecated. Use {name} instead")

                return _gather_returns(name, lhs, algm)

        # Set the signature of the callable to be one that is only generated on request.
        Wrapper.__call__.__signature__ = LazyFunctionSignature(alg_name=name)

        wrapper = Wrapper(algm_alias=alias)
        wrapper.__name__ = name
        return wrapper

    # Register aliases - split on whitespace
    for alias in algm_object.alias().strip().split():
        globals()[alias] = algorithm_wrapper(alias=alias)

    algm_wrapper = algorithm_wrapper()
    globals()[name] = algorithm_wrapper()  # Insert definition in to global dict
    return algm_wrapper


# -------------------------------------------------------------------------------------------------------------


def _create_algorithm_object(name, version=-1, startProgress=None, endProgress=None):
    """
    Create and initialize the named algorithm of the given version. This
    method checks whether the function call has come from within a PyExec
    call. If that is the case then an unmanaged child algorithm is created.

    :param name A string name giving the algorithm
    :param version A int version number
    """
    import inspect

    parent = _find_parent_pythonalgorithm(inspect.currentframe())
    if parent is not None:
        kwargs = {"version": version}
        if (startProgress is not None) and (endProgress is not None):
            kwargs["startProgress"] = float(startProgress)
            kwargs["endProgress"] = float(endProgress)
        alg = parent.createChildAlgorithm(name, **kwargs)
    else:
        # managed algorithm so that progress reporting
        # can be more easily wired up automatically
        alg = AlgorithmManager.create(name, version)
    # common traits
    alg.setRethrows(True)
    return alg


# -------------------------------------------------------------------------------------------------------------


def _find_parent_pythonalgorithm(frame):
    """
    Look for a PyExec method in the call stack and return
    the self object that the method is attached to

    :param frame The starting frame for the stack walk
    :returns The self object that is running the PyExec method
             or None if one was not found
    """
    # We are looking for this method name
    fn_name = "PyExec"

    # Return the 'self' object of a given frame
    def get_self(frame_arg):
        return frame_arg.f_locals["self"]

    # Look recursively for the PyExec method in the stack
    if frame.f_code.co_name == fn_name:
        return get_self(frame)
    while True:
        if frame.f_back:
            if frame.f_back.f_code.co_name == fn_name:
                return get_self(frame.f_back)
            frame = frame.f_back
        else:
            break
    if frame.f_code.co_name == fn_name:
        return get_self(frame)
    else:
        return None


# ----------------------------------------------------------------------------------------------------------------------


def _create_fake_function(name):
    """Create fake functions for the given name"""

    # ------------------------------------------------------------------------------------------------
    def fake_function(*args, **kwargs):
        raise RuntimeError(
            "Mantid import error. The mock simple API functions have not been replaced!"
            " This is an error in the core setup logic of the mantid module, "
            "please contact the development team."
        )

    # ------------------------------------------------------------------------------------------------
    fake_function.__name__ = name
    _replace_signature(fake_function, ("", ""))
    globals()[name] = fake_function


def _mockup(plugins):
    """
    Creates fake, error-raising functions for any plugins given.
    The function name for the Python algorithms are taken from the filename
    so this mechanism requires the algorithm name to match the filename.
    This mechanism solves the "chicken-and-egg" problem with Python algorithms trying
    to use other Python algorithms through the simple API functions. The issue
    occurs when a python algorithm tries to import the simple API function of another
    Python algorithm that has not been loaded yet, usually when it is further along
    in the alphabet. The first algorithm stops with an import error as that function
    is not yet known. By having a pre-loading step all of the necessary functions
    on this module can be created and after the plugins are loaded the correct
    function definitions can overwrite the "fake" ones.
    :param plugins: A list of  modules that have been loaded
    """
    module_attrs = globals()

    def create_fake_function(func_name):
        """Create fake functions for the given func_name"""

        # ------------------------------------------------------------------------------------------------
        def fake_function(*args, **kwargs):
            raise RuntimeError(
                "Mantid import error. The mock simple API functions have not been replaced!"
                " This is an error in the core setup logic of the mantid module, "
                "please contact the development team."
            )

        # ------------------------------------------------------------------------------------------------
        if "." in func_name:
            func_name = func_name.rstrip(".py")
        if specialization_exists(func_name):
            return
        fake_function.__name__ = func_name
        module_attrs[func_name] = fake_function

    for plugin in plugins:
        name = os.path.basename(plugin)
        name = os.path.splitext(name)[0]
        create_fake_function(name)


def _translate():
    """
    Loop through the algorithms and register a function call
    for each of them
    :returns: a list of the name of new function calls
    """
    from mantid.api import AlgorithmFactory, AlgorithmManager

    new_func_attrs = []
    # Method names mapped to their algorithm names. Used to detect multiple copies of same method name
    # on different algorithms, which is an error
    new_methods = {}

    algs = AlgorithmFactory.getRegisteredAlgorithms(True)
    algorithm_mgr = AlgorithmManager
    for name, versions in algs.items():
        if specialization_exists(name):
            continue
        try:
            # Create the algorithm object
            algm_object = algorithm_mgr.createUnmanaged(name, max(versions))
        except Exception as exc:
            logger.warning("Error initializing {0} on registration: '{1}'".format(name, str(exc)))
            continue

        algorithm_wrapper = _create_algorithm_function(name, max(versions), algm_object)
        method_name = algm_object.workspaceMethodName()
        if len(method_name) > 0:
            if method_name in new_methods:
                other_alg = new_methods[method_name]
                raise RuntimeError(
                    "simpleapi: Trying to attach '%s' as method to point to '%s' algorithm but "
                    "it has already been attached to point to the '%s' algorithm.\n"
                    "Does one inherit from the other? "
                    "Please check and update one of the algorithms accordingly." % (method_name, algm_object.name(), other_alg)
                )
            _attach_algorithm_func_as_method(method_name, algorithm_wrapper, algm_object)
            new_methods[method_name] = algm_object.name()
        new_func_attrs.append(name)

    return new_func_attrs


# -------------------------------------------------------------------------------------------------------------


def _attach_algorithm_func_as_method(method_name, algorithm_wrapper, algm_object):
    """
    Attachs the given algorithm free function to those types specified by the algorithm
    :param method_name: The name of the new method on the type
    :param algorithm_wrapper: Function object whose signature should be f(*args,**kwargs) and when
                             called will run the selected algorithm
    :param algm_object: An algorithm object that defines the extra properties of the new method
    """
    input_prop = algm_object.workspaceMethodInputProperty()
    if input_prop == "":
        raise RuntimeError(
            "simpleapi: '%s' has requested to be attached as a workspace method but "
            "Algorithm::workspaceMethodInputProperty() has returned an empty string."
            "This method is required to map the calling object to the correct property." % algm_object.name()
        )
    _api._workspaceops.attach_func_as_method(
        method_name, algorithm_wrapper, input_prop, algm_object.name(), algm_object.workspaceMethodOn()
    )


@contextmanager
def _update_sys_path(dirs):
    """
    Temporarily update the system path with a list of directories
    :param dirs: List of directories to add.
    """
    for dir_path in dirs:
        sys.path.append(dir_path)
    try:
        yield
    finally:
        for dir_path in dirs:
            sys.path.remove(dir_path)


# Initialization:
#   - start FrameworkManager (if necessary). The check is necessary as
#    _FrameworkManagerImpl.Instance() will import this module and deadlock if it
#    calls Instance again while importing this module
#   - loads the python plugins and create new algorithm functions
if not _api.FrameworkManagerImpl.hasInstance():
    _api.FrameworkManagerImpl.Instance()
_translate()

# Load the Python plugins
# The exported C++ plugins
from . import _plugins  # noqa

# Now the algorithms
# There is a chicken and egg problem with what we want to achieve here.
# The simpleapi module should contain function definitions for all algorithms
# and fit function classes but a python plugin can choose to import
# simpleapi itself before we have been finished initializing the module
# and creating a circular dependency. The only way to avoid this is to
# restrict the usage of simpleapi in Python plugins so that
# 'from simpleapi import *' is banned and all access is through
# 'import mantid.simpleapi as sapi'

# Set the .simpleapi attribute on the 'mantid' module before importing
# the plugins. Python usual does this once the module has been fully imported
# but we need to do this earlier
setattr(mantid, MODULE_NAME, sys.modules["mantid.{}".format(MODULE_NAME)])
try:
    _user_key = "user.python.plugins.directories"
    _user_plugin_dirs = _plugin_helper.get_plugin_paths_as_set(_user_key)
    # Use a cmake generated manifest of all the built in python algorithms to load them into the api
    plugins_manifest_path = ConfigService.Instance()["python.plugins.manifest"]
    plugins_dir = os.path.dirname(plugins_manifest_path)
    _plugin_files = []
    _plugin_dirs = set()
    if not plugins_manifest_path:
        logger.information("Path to plugins manifest is empty. The python plugins will not be loaded.")
    elif not os.path.exists(plugins_manifest_path):
        logger.warning(
            "The path to the python plugins manifest is invalid. The built in python plugins will " "not be loaded into the simpleapi."
        )
    else:
        with open(plugins_manifest_path) as manifest:
            plugin_paths = manifest.read().splitlines()
            for path in plugin_paths:
                plugin_name = os.path.splitext(path)[0]
                if not os.path.isabs(path):
                    path = os.path.join(plugins_dir, path)
                _plugin_dirs.add(os.path.dirname(path))
                _plugin_files.append(path)

    # Look for and import the user plugins
    for directory in _user_plugin_dirs:
        try:
            plugins, _ = _plugin_helper.find_plugins(directory)
            _plugin_files.extend(plugins)
            _plugin_dirs.add(directory)
        except ValueError as e:
            logger.warning(f"Error occurred during plugin discovery: {str(e)}")
            continue

    # Mock out the expected functions
    _mockup(_plugin_files)
    # Load the plugins.
    with _update_sys_path(_plugin_dirs):
        _plugin_modules = _plugin_helper.load(_plugin_files)
    # Create the final proper algorithm definitions for the plugins
    _plugin_attrs = _translate()
    # Finally, overwrite the mocked function definitions in the loaded modules with the real ones
    _plugin_helper.sync_attrs(globals(), _plugin_attrs, _plugin_modules)

    # Attach fit function wrappers
    from .fitfunctions import _wrappers

    _globals = globals()
    _globals.update(_wrappers())
except Exception:
    # If an error gets raised remove the attribute to be consistent
    # with standard python behaviour and reraise the exception
    delattr(mantid, MODULE_NAME)
    raise
