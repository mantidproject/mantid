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
import api
import kernel
from kernel import funcreturns as _funcreturns
from api import AnalysisDataService as _ads
from api import FrameworkManager as _framework

# This is a simple API so give access to the aliases by default as well
from mantid import apiVersion, __gui__
from kernel._aliases import *
from api._aliases import *

#------------------------ Specialized function calls --------------------------

__SPECIALIZED_FUNCTIONS__ = ["Load", "Fit"]

def specialization_exists(name):
    """
        Returns true if a specialization for the given name
        already exists, false otherwise
        
        @param name :: The name of a possible new function
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
      Load('INSTR00001000.nxs', 'run_ws')
      
      # ISIS NeXus with SpectrumMin and SpectrumMax = 1
      Load('INSTR00001000.nxs', 'run_ws', SpectrumMin=1,SpectrumMax=1)
      
      # SNS Event NeXus with precount on
      Load('INSTR_1000_event.nxs', 'event_ws', Precount=True)
      
      # A mix of keyword and non-keyword is also possible
      Load('event_ws', Filename='INSTR_1000_event.nxs',Precount=True)
    """
    filename, = get_mandatory_args('Load', ["Filename"], *args, **kwargs)
    
    # Create and execute
    algm = _framework.createAlgorithm('Load')
    algm.setProperty('Filename', filename) # Must be set first
    # Remove from keywords so it is not set twice
    try:
        del kwargs['Filename']
    except KeyError:
        pass
    lhs = _funcreturns.lhs_info(use_object_names=True)
    # If the output has not been assigned to anything, i.e. lhs[0] = 0 and kwargs does not have OutputWorkspace
    # then raise a more helpful error than what we would get from an algorithm
    if lhs[0] == 0 and 'OutputWorkspace' not in kwargs:
        raise RuntimeError("Unable to set output workspace name. Please either assign the output of "
                           "Load to a variable or use the OutputWorkspace keyword.") 
    
    lhs_args = get_args_from_lhs(lhs, algm)
    final_keywords = merge_keywords_with_lhs(kwargs, lhs_args)
    # Check for any properties that aren't known and warn they will not be used
    for key in final_keywords.keys():
        if key not in algm:
            logger.warning("You've passed a property (%s) to Load() that doesn't apply to this file type." % key)
            del final_keywords[key]
    _set_properties(algm, **final_keywords)
    algm.execute()
        
    # If a WorkspaceGroup was loaded then there will be OutputWorkspace_ properties about, don't include them
    return gather_returns('Load', lhs, algm, ignore_regex=['LoaderName','OutputWorkspace_.*'])

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
      - Disable :: A CSV list of properties to keep enabled in the dialog
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
    
    algm = _framework.createAlgorithm('Load')
    _set_properties_dialog(algm,**arguments)
    algm.execute()
    return algm

#---------------------------- Fit ---------------------------------------------

def Fit(*args, **kwargs):
    """
    Fit defines the interface to the fitting framework within Mantid.
    It can work with arbitrary data sources and therefore some options
    are only available when the function & workspace type are known.
    
    This simple wrapper takes the Function (as a string) & the InputWorkspace
    as the first two arguments. The remaining arguments must be 
    specified by keyword.
    
    Example:
      Fit(Function='name=LinearBackground,A0=0.3', InputWorkspace=dataWS',
          StartX='0.05',EndX='1.0',Output="Z1")
    """
    Function, InputWorkspace = get_mandatory_args('Fit', ["Function", "InputWorkspace"], *args, **kwargs)
    # Check for behaviour consistent with old API
    if type(Function) == str and Function in _ads:
        raise ValueError("Fit API has changed. The function must now come first in the argument list and the workspace second.")
    # Create and execute
    algm = _framework.createAlgorithm('Fit')
    algm.setProperty('Function', Function) # Must be set first
    algm.setProperty('InputWorkspace', InputWorkspace)
    # Remove from keywords so it is not set twice
    try:
        del kwargs['Function']
        del kwargs['InputWorkspace']
    except KeyError:
        pass
    
    lhs = _funcreturns.lhs_info(use_object_names=True)
    # Check for any properties that aren't known and warn they will not be used
    for key in kwargs.keys():
        if key not in algm:
            logger.warning("You've passed a property (%s) to Fit() that doesn't apply to this file type." % key)
            del kwargs[key]
    _set_properties(algm, **kwargs)
    algm.execute()
    
    return gather_returns('Fit', lhs, algm)

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
      - Disable :: A CSV list of properties to keep enabled in the dialog
      - Message :: An optional message string
    """
    arguments = {}
    try:
        function, inputworkspace = get_mandatory_args('FitDialog', ['Function', 'InputWorkspace'], *args, **kwargs)
        arguments['Function'] = function
        arguments['InputWorkspace'] = inputworkspace
    except RuntimeError:
        pass
    arguments.update(kwargs)
    if 'Enable' not in arguments: arguments['Enable']=''
    if 'Disable' not in arguments: arguments['Disable']=''
    if 'Message' not in arguments: arguments['Message']=''
    
    algm = _framework.createAlgorithm('Fit')
    _set_properties_dialog(algm,**arguments)
    algm.execute()
    return algm


def get_mandatory_args(func_name, required_args ,*args, **kwargs):
    """
    Given a list of required arguments, parse them
    from the given args & kwargs and raise an error if they
    are not provided
    
        @param func_name :: The name of the function call
        @param required_args :: A list of names of required arguments
        @param args :: The positional arguments to check
        @param kwargs :: The keyword arguments to check
        
        @returns A tuple of provided mandatory arguments
    """
    def get_argument_value(key, kwargs):
        try:
            value = kwargs[key]
            del kwargs[key]
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
        
        @param prop - A property object
        @returns True if the property is considered to be of type workspace
    """
    if isinstance(prop, api.IWorkspaceProperty):
        return True
    if 'Workspace' in prop.name: return True
    # Doesn't look like a workspace property
    return False

def get_args_from_lhs(lhs, algm_obj):
    """
        Return the extra arguments that are to be passed to the algorithm
        from the information in the lhs tuple. These are basically the names 
        of output workspaces.
        The algorithm properties are iterated over in the same order
        they were created within the wrapper and for each output
        workspace property an entry is added to the returned dictionary
        that contains {PropertyName:lhs_name}.
        
        @param lhs :: A 2-tuple that contains the number of variables supplied 
                      on the lhs of the function call and the names of these
                      variables
        @param algm_obj :: An initialised algorithm object
        @returns A dictionary mapping property names to the values
                 extracted from the lhs variables
    """
    ret_names = lhs[1]
    extra_args = {}

    output_props = [ algm_obj.getProperty(p) for p in algm_obj.outputProperties() ]
    nprops = len(output_props)
    i = 0
    while len(ret_names) > 0 and i < nprops:
        p = output_props[i]
        #
        # Some algorithms declare properties as EventWorkspace and not its interface
        # so Python doesn't know about it. Need to find a better way of
        # dealing with WorkspaceProperty types. If they didn't multiple inherit
        # that would help
        if _is_workspace_property(p):
            extra_args[p.name] = ret_names[0]
            ret_names = ret_names[1:]
        i += 1
    return extra_args

def merge_keywords_with_lhs(keywords, lhs_args):
    """
        Merges the arguments from the two dictionaries specified
        by the keywords passed to a function and the lhs arguments
        that have been parsed. Any value in keywords overrides on
        in lhs_args.
        
        @param keywords :: A dictionary of keywords that has been 
                           passed to the function call
        @param lhs_args :: A dictionary of arguments retrieved from the lhs
                           of the function call
    """
    final_keywords = lhs_args
    final_keywords.update(keywords)
    return final_keywords
    
def gather_returns(func_name, lhs, algm_obj, ignore_regex=[]):
    """
        Gather the return values and ensure they are in the
        correct order as defined by the output properties and
        return them as a tuple. If their is a single return
        value it is returned on its own
        
        @param func_name :: The name of the calling function
        @param lhs :: A 2-tuple that contains the number of variables supplied 
               on the lhs of the function call and the names of these
               variables
        @param algm_obj :: An executed algorithm object
        @param ignore_regex :: A list of strings containing regex expressions to match against property names
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
        # Child algorithms store their workspaces in the property
        if not algm_obj.isChild() and _is_workspace_property(prop):
            retvals.append(_ads[prop.valueAsStr])
        else:
            if not hasattr(prop, 'value'):
                print ('Unknown property type encountered. "%s" on "%s" is not understood by '
                       'Python. Return value skipped. Please contact development team' % (name, algm_obj.name()))
            else:
                retvals.append(prop.value)
    nvals = len(retvals)
    nlhs = lhs[0]
    if nlhs > 1 and nvals != nlhs:
        # There is a discrepancy in the number are unpacking variables
        # Let's not have the more cryptic unpacking error raised
        raise RuntimeError("%s is trying to return %d output(s) but you have provided %d variable(s). These numbers must match." % (func_name, nvals, nlhs))
    if nvals > 1:
        return tuple(retvals) # Create a tuple
    elif nvals == 1:
        return retvals[0]
    else:
        return None

def _set_properties(alg_object, *args, **kwargs):
    """
        Set all of the properties of the algorithm
        @param alg_object An initialised algorithm object
        @param *args Positional arguments
        @param **kwargs Keyword arguments  
    """
    prop_order = alg_object.mandatoryProperties()
    # add the args to the kw list so everything can be set in a single way
    for (key, arg) in zip(prop_order[:len(args)], args):
        kwargs[key] = arg

    # Set the properties of the algorithm.
    for key in kwargs.keys():
        value = kwargs[key]
        # Anything stored in the ADS must be set by string value
        # if it is not a child algorithm. 
        if (not alg_object.isChild()) and isinstance(value, kernel.DataItem):
            alg_object.setPropertyValue(key, value.name())
        else:
            alg_object.setProperty(key, value)

def create_algorithm(algorithm, version, _algm_object):
    """
        Create a function that will set up and execute an algorithm.
        The help that will be displayed is that of the most recent version.
        @param algorithm: name of the algorithm
        @param _algm_object :: the created algorithm object.
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
        algm = _framework.createAlgorithm(algorithm, _version)
        lhs = _funcreturns.lhs_info(use_object_names=True)
        lhs_args = get_args_from_lhs(lhs, algm)
        final_keywords = merge_keywords_with_lhs(kwargs, lhs_args)
        _set_properties(algm, *args, **final_keywords)
        algm.execute()
        return gather_returns(algorithm, lhs, algm)
        
    
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
        if len(str(prop.isValid))>0:
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
            
def _set_properties_dialog(algm_object, *args, **kwargs):
    """
    Set the properties all in one go assuming that you are preparing for a
    dialog box call. If the dialog is cancelled raise a runtime error, otherwise 
    return the algorithm ready to execute.
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
           isinstance(value, kernel.std_vector_dbl) or \
           isinstance(value, kernel.std_vector_int) or \
           isinstance(value, kernel.std_vector_long) or \
           isinstance(value, kernel.std_vector_size_t):
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
    import _qti
    dialog =  _qti.app.mantidUI.createPropertyInputDialog(algm_object.name(), presets, message, enabled_list, disabled_list)
    if dialog == False:
        raise RuntimeError('Information: Script execution cancelled')

def create_algorithm_dialog(algorithm, version, _algm_object):
    """
        Create a function that will set up and execute an algorithm dialog.
        The help that will be displayed is that of the most recent version.
        @param algorithm: name of the algorithm
        @param _algm_object :: the created algorithm object.
    """
    def algorithm_wrapper(*args, **kwargs):
        _version = version
        if "Version" in kwargs:
            _version = kwargs["Version"]
            del kwargs["Version"]
        for item in ["Message", "Enable", "Disable"]:
            if item not in kwargs:
                kwargs[item] = ""
            
        algm = _framework.createAlgorithm(algorithm, _version)
        _set_properties_dialog(algm, *args, **kwargs)
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



#==============================================================================

def translate():
    """
        Loop through the algorithms and register a function call 
        for each of them
    """
    from api import AlgorithmFactory, AlgorithmManager
     
    algs = AlgorithmFactory.getRegisteredAlgorithms(True)
    algorithm_mgr = AlgorithmManager
    for name, versions in algs.iteritems():
        if specialization_exists(name):
            continue
        try:
            # Create the algorithm object
            _algm_object = algorithm_mgr.createUnmanaged(name, max(versions))
            _algm_object.initialize()
        except Exception:
            continue
        create_algorithm(name, max(versions), _algm_object)
        create_algorithm_dialog(name, max(versions), _algm_object)

def mockout_api():
    """
        Creates fake, error-raising functions for all currently loaded algorithm
        plugins, including Python algorithms.
    """
    def create_fake_function(plugins):
        def fake_function(*args, **kwargs):
            raise RuntimeError("Mantid import error. The mock simple API functions have not been replaced!" +
                               " This is an error in the core setup logic of the mantid module, please contact the development team.")
        
        for name in plugins:
                if name.endswith('.py'):
                    name = name.rstrip('.py')
                if specialization_exists(name):
                    continue
                fake_function.__name__ = name
                if name not in globals():
                    globals()[name] = fake_function
    #
    from api import AlgorithmFactory
    import os
    cppalgs = AlgorithmFactory.getRegisteredAlgorithms(True)
    create_fake_function(cppalgs.keys())
    
    directories = kernel.config["pythonalgorithms.directories"].split(';')
    for top_dir in directories:
        for root, dirs, files in os.walk(top_dir):
            create_fake_function(files)

