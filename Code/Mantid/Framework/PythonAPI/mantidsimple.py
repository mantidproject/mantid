"""
    Code used to build the mantidsimple module in memory.
"""
from MantidFramework import *
from MantidFramework import _makeString

def version():
    return "mantidsimple - memory-based version"

def numberRows(descr, fw):
  des_len = len(descr)
  if des_len == 0:
    return (1, [''])
  nrows = 0
  i = 0
  descr_split = []
  while i < des_len:
    nrows += 1
    descr_split.append(descr[i:i+fw])
    i += fw
  return (nrows, descr_split)
  
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

        # Warn about old API
        warnOnV1MethodCall(inspect.currentframe().f_back, identifier=algorithm)
        algm = mtd.createAlgorithm(algorithm, _version)
        algm.setPropertyValues(*args, **kwargs)
        algm.execute()
        return algm
    
    algorithm_wrapper.__name__ = algorithm
    
    # This creates/initializes the algorithm once to make the documentation
    algorithm_wrapper.__doc__ = mtd.createAlgorithmDocs(algorithm, version)
    
    # Dark magic to get the correct function signature
    # Calling help(...) on the wrapper function will produce a function 
    # signature along the lines of AlgorithmName(*args, **kwargs).
    # We will replace the name "args" by the list of properties, and
    # the name "kwargs" by "Version=1".
    #   1- Get the algorithm properties and build a string to list them,
    #      taking care of giving no default values to mandatory parameters
    
    #_algm_object = mtd.createUnmanagedAlgorithm(algorithm, version)
    arg_list = []
    for p in mtd._getPropertyOrder(_algm_object):
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
        # Warn about old API
        warnOnV1MethodCall(inspect.currentframe().f_back, identifier=algorithm)
        algm = mtd.createAlgorithm(algorithm, _version)
        algm.setPropertiesDialog(*args, **kwargs)
        algm.execute()
        return algm
    
    algorithm_wrapper.__name__ = "%sDialog" % algorithm
    algorithm_wrapper.__doc__ = "\n\n%s dialog" % algorithm

    # Dark magic to get the correct function signature
    #_algm_object = mtd.createUnmanagedAlgorithm(algorithm, version)
    arg_list = []
    for p in mtd._getPropertyOrder(_algm_object):
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
    filename, wkspace = get_mandatory_args('Load', ['Filename', 'OutputWorkspace'], *args, **kwargs)
    
    # Create and execute
    # Warn about old API
    warnOnV1MethodCall(inspect.currentframe().f_back, identifier="Load")
    algm = mtd.createAlgorithm('Load')
    algm.setPropertyValue('Filename', filename) # Must be set first
    algm.setPropertyValue('OutputWorkspace', wkspace)
    # Remove them from the kwargs if they are there so they are not set twice
    try:
        del kwargs['Filename']
        del kwargs['OutputWorkspace']
    except KeyError:
        pass
    for key, value in kwargs.iteritems():
        try:
            algm.setPropertyValue(key, _makeString(value).lstrip('? '))
        except RuntimeError:
            mtd.sendWarningMessage("You've passed a property (%s) to Load() that doesn't apply to this filetype."% key)
    algm.execute()
    return algm

# Have a better load signature for autocomplete
_signature = "\bFilename,OutputWorkspace"
# Getting the code object for Load
_f = Load.func_code
# Creating a new code object nearly identical, but with the two variable names replaced
# by the property list.
_c = _f.__new__(_f.__class__, _f.co_argcount, _f.co_nlocals, _f.co_stacksize, _f.co_flags, _f.co_code, _f.co_consts, _f.co_names,\
       (_signature, "kwargs"), _f.co_filename, _f.co_name, _f.co_firstlineno, _f.co_lnotab, _f.co_freevars)
# Replace the code object of the wrapper function
Load.func_code = _c

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

    # Warn about old API
    warnOnV1MethodCall(inspect.currentframe().f_back, identifier="LoadDialog")
    algm = mtd.createAlgorithm('Load')
    algm.setPropertiesDialog(**arguments)
    algm.execute()
    return algm

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
    if type(Function) == str and Function in mtd:
        raise ValueError("Fit API has changed. The function must now come first in the argument list and the workspace second.")
    
    # Create and execute
    # Warn about old API
    warnOnV1MethodCall(inspect.currentframe().f_back, identifier="Fit")
    algm = mtd.createAlgorithm('Fit')
    algm.setPropertyValue('Function', str(Function)) # Must be set first
    algm.setPropertyValue('InputWorkspace', str(InputWorkspace))
    try:
        del kwargs['Function']
        del kwargs['InputWorkspace']
    except KeyError:
        pass
    for key, value in kwargs.iteritems():
        try:
            algm.setPropertyValue(key, _makeString(value).lstrip('? '))
        except RuntimeError:
            mtd.sendWarningMessage("You've passed a property (%s) to Fit() that doesn't apply to this workspace type." % key)
    algm.execute()
    return algm

# Have a better load signature for autocomplete
_signature = "\bFunction,InputWorkspace"
# Getting the code object for Load
_f = Fit.func_code
# Creating a new code object nearly identical, but with the two variable names replaced
# by the property list.
_c = _f.__new__(_f.__class__, _f.co_argcount, _f.co_nlocals, _f.co_stacksize, _f.co_flags, _f.co_code, _f.co_consts, _f.co_names,\
       (_signature, "kwargs"), _f.co_filename, _f.co_name, _f.co_firstlineno, _f.co_lnotab, _f.co_freevars)
# Replace the code object of the wrapper function
Fit.func_code = _c

def FitDialog(*args, **kwargs):
    """Popup a dialog for the Fit algorithm. More help on the Fit function
    is available via help(Fit).

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

    # Warn about old API
    warnOnV1MethodCall(inspect.currentframe().f_back, identifier="FitDialog")
    algm = mtd.createAlgorithm('Fit')
    algm.setPropertiesDialog(**arguments)
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

#-------------------------------------------------------------------------------------------------------------------

def mockup(directories):
    """
        Creates fake, error-raising functions for all loaded algorithms plus
        any python algorithms in the given directories. 
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
    """
    #--------------------------------------------------------------------------------------------------------
    def create_fake_functions(algs):
        """Create fake functions for all of the listed names
        """
        #----------------------------------------------------------------------------------------------------
        def create_fake(name):
            """Create fake functions for the given name
            """
            #------------------------------------------------------------------------------------------------
            def fake_function(*args, **kwargs):
                raise RuntimeError("Mantid import error. The mock mantidsimple functions have not been replaced!" +
                                   " This is an error in the core setup logic of the mantidsimple module, please contact the development team.")
            #------------------------------------------------------------------------------------------------
            if "." in name:
                if name.endswith('.py'):
                    name = name.rstrip('.py')
                else:
                    return
            if specialization_exists(name):
                return
            fake_function.__name__ = name
            f = fake_function.func_code
            c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, f.co_consts, f.co_names,\
                          ("", ""), f.co_filename, f.co_name, f.co_firstlineno, f.co_lnotab, f.co_freevars)
            # Replace the code object of the wrapper function
            fake_function.func_code = c
            globals()[name] = fake_function
        #----------------------------------------------------------------------------------------------------
        for algorithm in algs:
            if type(algorithm) == tuple:
                alg_name = algorithm[0]
            else:
                alg_name = algorithm
            create_fake(alg_name)
    #--------------------------------------------------------------------------------------------------------
    # Start with the loaded C++ algorithms
    import os
    cppalgs = mtd._getRegisteredAlgorithms(include_hidden=True)
    create_fake_functions(cppalgs)
    
    # Now the plugins
    if type(directories) != list:
        directories = [directories]
    for top_dir in directories:
        for root, dirs, filenames in os.walk(top_dir):
            if 'functions' in root: # Functions are solely for new API
                continue
            create_fake_functions(filenames)

#------------------------------------------------------------------------------
# Flag to indicate if warnings should be issued regarding errors on loading 
__ISSUE_WARNINGS = True

def translate():
    """
        Loop through the algorithms and register a function call 
        for each of them
    """
    global __ISSUE_WARNINGS
    new_attrs = []
    for algorithm in mtd._getRegisteredAlgorithms(include_hidden=True):
        name = algorithm[0]
        if specialization_exists(name):
            continue
        highest_version = max(algorithm[1])
        try:
            # Create the algorithm object
            _algm_object = mtd.createUnmanagedAlgorithm(name, max(algorithm[1]))
            create_algorithm(name, highest_version, _algm_object)
            create_algorithm_dialog(name, highest_version, _algm_object)
            new_attrs.append(name)
        except Exception, exc:
            if __ISSUE_WARNINGS: 
                mtd.sendWarningMessage("Cannot load '%s' algorithm. Error='%s'" % (name,str(exc)))
            continue
    # After this has run once, turn the warnings off 
    __ISSUE_WARNINGS = False
    return new_attrs

translate()