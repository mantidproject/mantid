"""
    This module defines a simple function-style API for running Mantid
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
    and assign it to the rebinned variable

"""
from __future__ import absolute_import

import mantid.api as _api
import mantid.kernel as _kernel

# This is a simple API so give access to the aliases by default as well
from mantid import apiVersion, __gui__
from mantid.kernel._aliases import *
from mantid.api._aliases import *

#------------------------ Specialized function calls --------------------------
# List of specialized algorithms
__SPECIALIZED_FUNCTIONS__ = ["Load", "Fit"]
# List of specialized algorithms
__MDCOORD_FUNCTIONS__ = ["PeakIntensityVsRadius", "CentroidPeaksMD","IntegratePeaksMD"]
# The "magic" keyword to enable/disable logging
__LOGGING_KEYWORD__ = "EnableLogging"

def specialization_exists(name):
    """
        Returns true if a specialization for the given name
        already exists, false otherwise

        :param name: The name of a possible new function
    """
    return name in __SPECIALIZED_FUNCTIONS__

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
    filename, = _get_mandatory_args('Load', ["Filename"], *args, **kwargs)

    # Create and execute
    algm = _create_algorithm_object('Load')
    _set_logging_option(algm, kwargs)
    algm.setProperty('Filename', filename) # Must be set first
    # Remove from keywords so it is not set twice
    try:
        del kwargs['Filename']
    except KeyError:
        pass
    lhs = _kernel.funcreturns.lhs_info()
    # If the output has not been assigned to anything, i.e. lhs[0] = 0 and kwargs does not have OutputWorkspace
    # then raise a more helpful error than what we would get from an algorithm
    if lhs[0] == 0 and 'OutputWorkspace' not in kwargs:
        raise RuntimeError("Unable to set output workspace name. Please either assign the output of "
                           "Load to a variable or use the OutputWorkspace keyword.")

    lhs_args = _get_args_from_lhs(lhs, algm)
    final_keywords = _merge_keywords_with_lhs(kwargs, lhs_args)
    # Check for any properties that aren't known and warn they will not be used
    for key in final_keywords.keys():
        if key not in algm:
            logger.warning("You've passed a property (%s) to Load() that doesn't apply to this file type." % key)
            del final_keywords[key]
    _set_properties(algm, **final_keywords)
    algm.execute()

    # If a WorkspaceGroup was loaded then there will be a set of properties that have an underscore in the name
    # and users will simply expect the groups to be returned NOT the groups + workspaces.
    return _gather_returns('Load', lhs, algm, ignore_regex=['LoaderName','LoaderVersion','.*_.*'])

# Have a better load signature for autocomplete
_signature = "\bFilename"
# Getting the code object for Load
_f = Load.func_code
# Creating a new code object nearly identical, but with the two variable names replaced
# by the property list.
_c = _f.__new__(_f.__class__, _f.co_argcount, _f.co_nlocals, _f.co_stacksize, _f.co_flags, _f.co_code, _f.co_consts, _f.co_names,\
       (_signature, "kwargs"), _f.co_filename, _f.co_name, _f.co_firstlineno, _f.co_lnotab, _f.co_freevars)

# Replace the code object of the wrapper function
Load.func_code = _c
######################################################################

def LoadDialog(*args, **kwargs):
    """Popup a dialog for the Load algorithm. More help on the Load function
    is available via help(Load).

    Additional arguments available here (as keyword only) are:
      - Enable :: A CSV list of properties to keep enabled in the dialog
      - Disable :: A CSV list of properties to disable in the dialog
      - Message :: An optional message string
    """
    arguments = {}
    filename = None
    wkspace = None
    if len(args) == 2:
        filename = args[0]
        wkspace = args[1]
    elif len(args) == 1:
        if 'Filename' in kwargs:
            filename = kwargs['Filename']
            wkspace = args[0]
        elif 'OutputWorkspace' in kwargs:
            wkspace = kwargs['OutputWorkspace']
            filename = args[0]
    arguments['Filename'] = filename
    arguments['OutputWorkspace'] = wkspace
    arguments.update(kwargs)
    if 'Enable' not in arguments: arguments['Enable']=''
    if 'Disable' not in arguments: arguments['Disable']=''
    if 'Message' not in arguments: arguments['Message']=''

    algm = _create_algorithm_object('Load')
    _set_properties_dialog(algm,**arguments)
    algm.execute()
    return algm

#---------------------------- Fit ---------------------------------------------

def Fit(*args, **kwargs):
    """
    Fit defines the interface to the fitting 562 within Mantid.
    It can work with arbitrary data sources and therefore some options
    are only available when the function & workspace type are known.

    This simple wrapper takes the Function (as a string) & the InputWorkspace
    as the first two arguments. The remaining arguments must be
    specified by keyword.

    Example:
      Fit(Function='name=LinearBackground,A0=0.3', InputWorkspace=dataWS',
          StartX='0.05',EndX='1.0',Output="Z1")
    """
    Function, InputWorkspace = _get_mandatory_args('Fit', ["Function", "InputWorkspace"], *args, **kwargs)
    # Remove from keywords so it is not set twice
    if "Function" in kwargs:
        del kwargs['Function']
    if "InputWorkspace" in kwargs:
        del kwargs['InputWorkspace']

    # Check for behaviour consistent with old API
    if type(Function) == str and Function in _api.AnalysisDataService:
        raise ValueError("Fit API has changed. The function must now come first in the argument list and the workspace second.")
    # Create and execute
    algm = _create_algorithm_object('Fit')
    _set_logging_option(algm, kwargs)
    algm.setProperty('Function', Function) # Must be set first
    algm.setProperty('InputWorkspace', InputWorkspace)

    # Set all workspace properties before others
    for key in kwargs.keys():
        if key.startswith('InputWorkspace_'):
            algm.setProperty(key, kwargs[key])
            del kwargs[key]

    lhs = _kernel.funcreturns.lhs_info()
    # Check for any properties that aren't known and warn they will not be used
    for key in kwargs.keys():
        if key not in algm:
            logger.warning("You've passed a property (%s) to Fit() that doesn't apply to any of the input workspaces." % key)
            del kwargs[key]
    _set_properties(algm, **kwargs)
    algm.execute()

    return _gather_returns('Fit', lhs, algm)

# Have a better load signature for autocomplete
_signature = "\bFunction, InputWorkspace"
# Getting the code object for Load
_f = Fit.func_code
# Creating a new code object nearly identical, but with the two variable names replaced
# by the property list.
_c = _f.__new__(_f.__class__, _f.co_argcount, _f.co_nlocals, _f.co_stacksize, _f.co_flags, _f.co_code, _f.co_consts, _f.co_names,\
       (_signature, "kwargs"), _f.co_filename, _f.co_name, _f.co_firstlineno, _f.co_lnotab, _f.co_freevars)

# Replace the code object of the wrapper function
Fit.func_code = _c

def FitDialog(*args, **kwargs):
    """Popup a dialog for the Load algorithm. More help on the Load function
    is available via help(Load).

    Additional arguments available here (as keyword only) are:
      - Enable :: A CSV list of properties to keep enabled in the dialog
      - Disable :: A CSV list of properties to disable in the dialog
      - Message :: An optional message string
    """
    arguments = {}
    try:
        function, inputworkspace = _get_mandatory_args('FitDialog', ['Function', 'InputWorkspace'], *args, **kwargs)
        arguments['Function'] = function
        arguments['InputWorkspace'] = inputworkspace
    except RuntimeError:
        pass
    arguments.update(kwargs)
    if 'Enable' not in arguments: arguments['Enable']=''
    if 'Disable' not in arguments: arguments['Disable']=''
    if 'Message' not in arguments: arguments['Message']=''

    algm = _create_algorithm_object('Fit')
    _set_properties_dialog(algm,**arguments)
    algm.execute()
    return algm

#--------------------------------------------------- --------------------------

def _get_function_spec(func):
    """Get the python function signature for the given function object

    :param func: A Python function object
    """
    import inspect
    try:
        argspec = inspect.getargspec(func)
    except TypeError:
        return ''
    # Algorithm functions have varargs set not args
    args = argspec[0]
    if args != []:
        # For methods strip the self argument
        if hasattr(func, 'im_func'):
            args = args[1:]
        defs = argspec[3]
    elif argspec[1] is not None:
        # Get from varargs/keywords
        arg_str = argspec[1].strip().lstrip('\b')
        defs = []
        # Keyword args
        kwargs = argspec[2]
        if kwargs is not None:
            kwargs = kwargs.strip().lstrip('\b\b')
            if kwargs == 'kwargs':
                kwargs = '**' + kwargs + '=None'
            arg_str += ',%s' % kwargs
        # Any default argument appears in the string
        # on the rhs of an equal
        for arg in arg_str.split(','):
            arg = arg.strip()
            if '=' in arg:
                arg_token = arg.split('=')
                args.append(arg_token[0])
                defs.append(arg_token[1])
            else:
                args.append(arg)
        if len(defs) == 0: defs = None
    else:
        return ''

    if defs is None:
        calltip = ','.join(args)
        calltip = '(' + calltip + ')'
    else:
        # The defaults list contains the default values for the last n arguments
        diff = len(args) - len(defs)
        calltip = ''
        for index in range(len(args) - 1, -1, -1):
            def_index = index - diff
            if def_index >= 0:
                calltip = '[' + args[index] + '],' + calltip
            else:
                calltip = args[index] + "," + calltip
        calltip = '(' + calltip.rstrip(',') + ')'
    return calltip

#--------------------------------------------------- --------------------------

def _get_mandatory_args(func_name, required_args ,*args, **kwargs):
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
    def get_argument_value(key, kwargs):
        try:
            value = kwargs[key]
            return value
        except KeyError:
            raise RuntimeError('%s argument not supplied to %s function' % (str(key), func_name))
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
        reqd_as_str = ','.join(required_args).strip(",")
        raise RuntimeError('%s() takes "%s" as positional arguments. Other arguments must be specified by keyword.'
                           % (func_name, reqd_as_str))
    return tuple(mandatory_args)

#------------------------ General simple function calls ----------------------

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
    if type(prop) == _kernel.Property and 'Workspace' in prop.name:
        return True
    else:
        # Doesn't look like a workspace property
        return False

def _get_args_from_lhs(lhs, algm_obj):
    """
        Return the extra arguments that are to be passed to the algorithm
        from the information in the lhs tuple. These are basically the names
        of output workspaces.
        The algorithm properties are iterated over in the same order
        they were created within the wrapper and for each output
        workspace property an entry is added to the returned dictionary
        that contains {PropertyName:lhs_name}.

        :param lhs: A 2-tuple that contains the number of variables supplied on the lhs of the function call and the names of these variables
        :param algm_obj: An initialised algorithm object
        :returns: A dictionary mapping property names to the values extracted from the lhs variables
    """
    ret_names = lhs[1]
    extra_args = {}

    output_props = [ algm_obj.getProperty(p) for p in algm_obj.outputProperties() ]
    nprops = len(output_props)
    i = 0
    while len(ret_names) > 0 and i < nprops:
        p = output_props[i]
        if _is_workspace_property(p):
            extra_args[p.name] = ret_names[0]
            ret_names = ret_names[1:]
        i += 1
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

def _gather_returns(func_name, lhs, algm_obj, ignore_regex=[]):
    """Gather the return values and ensure they are in the
       correct order as defined by the output properties and
       return them as a tuple. If their is a single return
       value it is returned on its own

       :param func_name: The name of the calling function.
       :param lhs: A 2-tuple that contains the number of variables supplied on the lhs of the function call and the names of these variables.
       :param algm_obj: An executed algorithm object.
       :param ignore_regex: A list of strings containing regex expressions to match against property names that will be ignored & not returned.
    """
    import re
    def ignore_property(name, ignore_regex):
        for regex in ignore_regex:
            if regex.match(name) is not None:
                return True
        # Matched nothing
        return False

    if type(ignore_regex) is str:
        ignore_regex = [ignore_regex]
    # Compile regexes
    for index, expr in enumerate(ignore_regex):
        ignore_regex[index] = re.compile(expr)

    retvals = []
    for name in algm_obj.outputProperties():
        if ignore_property(name, ignore_regex):
            continue
        prop = algm_obj.getProperty(name)
        # Parent algorithms store their workspaces in the ADS
        # Child algorithms should store their workspaces in the property
        # but they don't at the moment while the issues with history recording Python Child Algs
        # is resolved: ticket #5157
        if _is_workspace_property(prop):
            value_str = prop.valueAsStr
            try:
                retvals.append(_api.AnalysisDataService[value_str])
            except KeyError:
                if not prop.isOptional():
                    raise RuntimeError("Internal error. Output workspace property '%s' on algorithm '%s' has not been stored correctly."
                                       "Please contact development team." % (name,  algm_obj.name()))
        else:
            if hasattr(prop, 'value'):
                retvals.append(prop.value)
            else:
                raise RuntimeError('Internal error. Unknown property type encountered. "%s" on algorithm "%s" is not understood by '
                       'Python. Please contact development team' % (name, algm_obj.name()))


    nvals = len(retvals)
    nlhs = lhs[0]
    if nlhs > 1 and nvals != nlhs:
        # There is a discrepancy in the number are unpacking variables
        # Let's not have the more cryptic unpacking error raised
        raise RuntimeError("%s is trying to return %d output(s) but you have provided %d variable(s). "
                           "These numbers must match." % (func_name, nvals, nlhs))
    if nvals > 1:
        return tuple(retvals) # Create a tuple
    elif nvals == 1:
        return retvals[0]
    else:
        return None

def _set_logging_option(algm_obj, kwargs):
    """
        Checks the keyword arguments for the _LOGGING keyword, sets the state of the
        algorithm logging accordingly and removes the value from the dictionary. If the keyword
        does not exist then it does nothing.

        :param alg_object: An initialised algorithm object
        :param **kwargs: A dictionary of the keyword arguments passed to the simple function call
    """
    if __LOGGING_KEYWORD__ in kwargs:
        algm_obj.setLogging(kwargs[__LOGGING_KEYWORD__])
        del kwargs[__LOGGING_KEYWORD__]

def _set_properties(alg_object, *args, **kwargs):
    """
        Set all of the properties of the algorithm
        :param alg_object: An initialised algorithm object
        :param *args: Positional arguments
        :param **kwargs: Keyword arguments
    """
    if len(args) > 0:
        mandatory_props = alg_object.mandatoryProperties()
        # Remove any already in kwargs
        for key in kwargs.keys():
            try:
                mandatory_props.remove(key)
            except ValueError:
                pass
        # If have any left
        if len(mandatory_props) > 0:
            # Now pair up the properties & arguments
            for (key, arg) in zip(mandatory_props[:len(args)], args):
                kwargs[key] = arg
        else:
            raise RuntimeError("Positional argument(s) provided but none are required. Check function call.")

    # Set the properties of the algorithm.
    for key in kwargs.keys():
        value = kwargs[key]
        if value is None:
            continue
        # The correct parent/child relationship is not quite set up yet: #5157
        # ChildAlgorithms in Python are marked as children but their output is in the
        # ADS meaning we cannot just set DataItem properties by value. At the moment
        # they are just set with strings
        if isinstance(value, _kernel.DataItem):
            alg_object.setPropertyValue(key, value.name())
        else:
            alg_object.setProperty(key, value)

def _create_algorithm_function(algorithm, version, _algm_object):
    """
        Create a function that will set up and execute an algorithm.
        The help that will be displayed is that of the most recent version.
        :param algorithm: name of the algorithm
        :param _algm_object: the created algorithm object.
    """

    def algorithm_wrapper(*args, **kwargs):
        """
            Note that if the Version parameter is passed, we will create
            the proper version of the algorithm without failing.
        """
        _version = version
        if "Version" in kwargs:
            _version = kwargs["Version"]
            del kwargs["Version"]
        algm = _create_algorithm_object(algorithm, _version)
        _set_logging_option(algm, kwargs)

        # Temporary removal of unneeded parameter from user's python scripts
        if "CoordinatesToUse" in kwargs and algorithm in __MDCOORD_FUNCTIONS__:
            del kwargs["CoordinatesToUse"]

        try:
            frame = kwargs["__LHS_FRAME_OBJECT__"]
            del kwargs["__LHS_FRAME_OBJECT__"]
        except KeyError:
            frame = None

        lhs = _kernel.funcreturns.lhs_info(frame=frame)
        lhs_args = _get_args_from_lhs(lhs, algm)
        final_keywords = _merge_keywords_with_lhs(kwargs, lhs_args)

        _set_properties(algm, *args, **final_keywords)
        algm.execute()
        return _gather_returns(algorithm, lhs, algm)


    algorithm_wrapper.__name__ = algorithm

    # Construct the algorithm documentation
    algorithm_wrapper.__doc__ = _algm_object.docString()

    # Dark magic to get the correct function signature
    # Calling help(...) on the wrapper function will produce a function
    # signature along the lines of AlgorithmName(*args, **kwargs).
    # We will replace the name "args" by the list of properties, and
    # the name "kwargs" by "Version=1".
    #   1 - Get the algorithm properties and build a string to list them,
    #       taking care of giving no default values to mandatory parameters
    #   2 - All output properties will be removed from the function
    #       argument list

    arg_list = []
    for p in _algm_object.mandatoryProperties():
        prop = _algm_object.getProperty(p)
        # Mandatory parameters are those for which the default value is not valid
        if isinstance(prop.isValid,str):
            valid_str = prop.isValid
        else:
            valid_str = prop.isValid()
        if len(valid_str) > 0:
            arg_list.append(p)
        else:
            # None is not quite accurate here, but we are reproducing the
            # behavior found in the C++ code for SimpleAPI.
            arg_list.append("%s=None" % p)

    # Build the function argument string from the tokens we found
    arg_str = ','.join(arg_list)
    # Calling help(...) will put a * in front of the first parameter name, so we use \b
    signature = "\b%s" % arg_str
    # Getting the code object for the algorithm wrapper
    f = algorithm_wrapper.func_code
    # Creating a new code object nearly identical, but with the two variable names replaced
    # by the property list.
    c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, f.co_consts, f.co_names,\
       (signature, "\b\bVersion=%d" % version), f.co_filename, f.co_name, f.co_firstlineno, f.co_lnotab, f.co_freevars)
    # Replace the code object of the wrapper function
    algorithm_wrapper.func_code = c

    globals()[algorithm] = algorithm_wrapper

    # Register aliases
    for alias in _algm_object.alias().strip().split(' '):
        alias = alias.strip()
        if len(alias)>0:
            globals()[alias] = algorithm_wrapper

    return algorithm_wrapper
#-------------------------------------------------------------------------------------------------------------

def _create_algorithm_object(name, version=-1):
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
        alg = parent.createChildAlgorithm(name, version)
        alg.setLogging(parent.isLogging()) # default is to log if parent is logging

        # Historic: simpleapi functions always put stuff in the ADS
        #           If we change this we culd potentially break many users' algorithms
        alg.setAlwaysStoreInADS(True)
    else:
        # managed algorithm so that progress reporting
        # can be more easily wired up automatically
        alg = AlgorithmManager.create(name, version)
    # common traits
    alg.setRethrows(True)
    return alg

#-------------------------------------------------------------------------------------------------------------

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
    def get_self(frame):
        return frame.f_locals['self']

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

#-------------------------------------------------------------------------------------------------------------

def _set_properties_dialog(algm_object, *args, **kwargs):
    """
    Set the properties all in one go assuming that you are preparing for a
    dialog box call. If the dialog is cancelled raise a runtime error, otherwise
    return the algorithm ready to execute.

    :param algm_object An initialized algorithm object
    """
    if not __gui__:
        raise RuntimeError("Can only display properties dialog in gui mode")

    # generic setup
    enabled_list = [s.lstrip(' ') for s in kwargs.get("Enable", "").split(',')]
    del kwargs["Enable"] # no longer needed
    disabled_list = [s.lstrip(' ') for s in kwargs.get("Disable", "").split(',')]
    del kwargs["Disable"] # no longer needed
    message = kwargs.get("Message", "")
    del kwargs["Message"]
    presets = '|'

    #-------------------------------------------------------------------------------
    def make_str(value):
        """Make a string out of a value such that the Mantid properties can understand it
        """
        import numpy

        if isinstance(value, numpy.ndarray):
            value = list(value) # Temp until more complete solution available (#2340)
        if isinstance(value, list) or \
           isinstance(value, _kernel.std_vector_dbl) or \
           isinstance(value, _kernel.std_vector_int) or \
           isinstance(value, _kernel.std_vector_long) or \
           isinstance(value, _kernel.std_vector_size_t):
            return str(value).lstrip('[').rstrip(']')
        elif isinstance(value, tuple):
            return str(value).lstrip('(').rstrip(')')
        elif isinstance(value, bool):
            if value:
                return '1'
            else:
                return '0'
        else:
            return str(value)
    # Translate positional arguments and add them to the keyword list
    ordered_props = algm_object.orderedProperties()
    for index, value in enumerate(args):
        propname = ordered_props[index]
        kwargs[propname] = args[index]

    # configure everything for the dialog
    for name in kwargs.keys():
        value = kwargs[name]
        if value is not None:
            presets += name + '=' + make_str(value) + '|'

    # finally run the configured dialog
    import mantidplot
    dialog_accepted =  mantidplot.createScriptInputDialog(algm_object.name(), presets, message, enabled_list, disabled_list)
    if not dialog_accepted:
        raise RuntimeError('Algorithm input cancelled')

#----------------------------------------------------------------------------------------------------------------------

def _create_algorithm_dialog(algorithm, version, _algm_object):
    """
        Create a function that will set up and execute an algorithm dialog.
        The help that will be displayed is that of the most recent version.
        :param algorithm: name of the algorithm
        :param _algm_object: the created algorithm object.
    """
    def algorithm_wrapper(*args, **kwargs):
        _version = version
        if "Version" in kwargs:
            _version = kwargs["Version"]
            del kwargs["Version"]
        for item in ["Message", "Enable", "Disable"]:
            if item not in kwargs:
                kwargs[item] = ""

        algm = _create_algorithm_object(algorithm, _version)
        _set_properties_dialog(algm, *args, **kwargs) # throws if input cancelled
        algm.execute()
        return algm

    algorithm_wrapper.__name__ = "%sDialog" % algorithm
    algorithm_wrapper.__doc__ = "\n\n%s dialog" % algorithm

    # Dark magic to get the correct function signature
    arg_list = []
    for p in _algm_object.orderedProperties():
        arg_list.append("%s=None" % p)
    arg_str = ','.join(arg_list)
    signature = "\b%s" % arg_str
    f = algorithm_wrapper.func_code
    c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, f.co_consts, f.co_names,\
       (signature, "\b\bMessage=\"\", Enable=\"\", Disable=\"\", Version=%d" % version), \
       f.co_filename, f.co_name, f.co_firstlineno, f.co_lnotab, f.co_freevars)
    algorithm_wrapper.func_code = c

    globals()["%sDialog" % algorithm] = algorithm_wrapper

    # Register aliases
    for alias in _algm_object.alias().strip().split(' '):
        alias = alias.strip()
        if len(alias)>0:
            globals()["%sDialog" % alias] = algorithm_wrapper

#--------------------------------------------------------------------------------------------------

def _mockup(plugins):
    """
        Creates fake, error-raising functions for all loaded algorithms plus
        any plugins given.
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
    #--------------------------------------------------------------------------------------------------------
    def create_fake_function(name):
        """Create fake functions for the given name
        """
        #------------------------------------------------------------------------------------------------
        def fake_function(*args, **kwargs):
            raise RuntimeError("Mantid import error. The mock simple API functions have not been replaced!" +
                               " This is an error in the core setup logic of the mantid module, please contact the development team.")
        #------------------------------------------------------------------------------------------------
        if "." in name:
            name = name.rstrip('.py')
        if specialization_exists(name):
            return
        fake_function.__name__ = name
        f = fake_function.func_code
        c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, f.co_consts, f.co_names,\
                      ("", ""), f.co_filename, f.co_name, f.co_firstlineno, f.co_lnotab, f.co_freevars)
        # Replace the code object of the wrapper function
        fake_function.func_code = c
        globals()[name] = fake_function
    #--------------------------------------------------------
    def create_fake_functions(alg_names):
        """Create fake functions for all of the listed names
        """
        for alg_name in alg_names:
            create_fake_function(alg_name)
    #-------------------------------------

    # Start with the loaded C++ algorithms
    from mantid.api import AlgorithmFactory
    import os
    cppalgs = AlgorithmFactory.getRegisteredAlgorithms(True)
    create_fake_functions(cppalgs.keys())

    # Now the plugins
    for plugin in plugins:
        name = os.path.basename(plugin)
        name = os.path.splitext(name)[0]
        create_fake_function(name)

#------------------------------------------------------------------------------------------------------------

def _translate():
    """
        Loop through the algorithms and register a function call
        for each of them

        :returns: a list of new function calls
    """
    from mantid.api import AlgorithmFactory, AlgorithmManager

    new_functions = [] # Names of new functions added to the global namespace
    new_methods = {} # Method names mapped to their algorithm names. Used to detect multiple copies of same method name
                     # on different algorithms, which is an error

    algs = AlgorithmFactory.getRegisteredAlgorithms(True)
    algorithm_mgr = AlgorithmManager
    for name, versions in algs.iteritems():
        if specialization_exists(name):
            continue
        try:
            # Create the algorithm object
            algm_object = algorithm_mgr.createUnmanaged(name, max(versions))
            algm_object.initialize()
        except Exception:
            continue

        algorithm_wrapper = _create_algorithm_function(name, max(versions), algm_object)
        method_name = algm_object.workspaceMethodName()
        if len(method_name) > 0:
            if method_name in new_methods:
                other_alg = new_methods[method_name]
                raise RuntimeError("simpleapi: Trying to attach '%s' as method to point to '%s' algorithm but "
                                   "it has already been attached to point to the '%s' algorithm.\n"
                                   "Does one inherit from the other? "
                                   "Please check and update one of the algorithms accordingly." % (method_name,algm_object.name(),other_alg))
            _attach_algorithm_func_as_method(method_name, algorithm_wrapper, algm_object)
            new_methods[method_name] = algm_object.name()

        # Dialog variant
        _create_algorithm_dialog(name, max(versions), algm_object)
        new_functions.append(name)

    return new_functions

#-------------------------------------------------------------------------------------------------------------

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
        raise RuntimeError("simpleapi: '%s' has requested to be attached as a workspace method but "
                           "Algorithm::workspaceMethodInputProperty() has returned an empty string."
                           "This method is required to map the calling object to the correct property." % algm_object.name())
    if input_prop not in algm_object:
        raise RuntimeError("simpleapi: '%s' has requested to be attached as a workspace method but "
                           "Algorithm::workspaceMethodInputProperty() has returned a property name that "
                           "does not exist on the algorithm." % algm_object.name())

    _api._workspaceops.attach_func_as_method(method_name, algorithm_wrapper, input_prop,
                                                  algm_object.workspaceMethodOn())
