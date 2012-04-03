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
    # Small inner function to grab the mandatory arguments and translate possible
    # exceptions
    def get_argument_value(key, kwargs):
        try:
            value = kwargs[key]
            del kwargs[key]
            return value
        except KeyError:
            raise RuntimeError('%s argument not supplied to Load function' % str(key))
    
    if len(args) == 2:
        filename = args[0]
        wkspace = args[1]
    elif len(args) == 1:
        if 'Filename' in kwargs:
            wkspace = args[0]
            filename = get_argument_value('Filename', kwargs)
        elif 'OutputWorkspace' in kwargs:
            filename = args[0]
            wkspace = get_argument_value('OutputWorkspace', kwargs)
        else:
            raise RuntimeError('Cannot find "Filename" or "OutputWorkspace" in key word list. '
                               'Cannot use single positional argument.')
    elif len(args) == 0:
        filename = get_argument_value('Filename', kwargs)
        wkspace = get_argument_value('OutputWorkspace', kwargs)
    else:
        raise RuntimeError('Load() takes at most 2 positional arguments, %d found.' % len(args))
    
    # Create and execute
    algm = mtd.createAlgorithm('Load')
    algm.setPropertyValue('Filename', filename) # Must be set first
    algm.setPropertyValue('OutputWorkspace', wkspace)
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
    algm = mtd.createAlgorithm('Load')
    algm.setPropertiesDialog(**arguments)
    algm.execute()
    return algm

def translate():
    """
        Loop through the algorithms and register a function call 
        for each of them
    """
    for algorithm in mtd._getRegisteredAlgorithms(include_hidden=True):
        if algorithm[0] == "Load":
            continue
        # Create the algorithm object
        _algm_object = mtd.createUnmanagedAlgorithm(algorithm[0], max(algorithm[1]))
        create_algorithm(algorithm[0], max(algorithm[1]), _algm_object)
        create_algorithm_dialog(algorithm[0], max(algorithm[1]), _algm_object)
            
def fake_python_alg_functions():
    """
        Creates fake, no-op functions to fool imports for
        Python algorithms. The real definitions will
        be created after all of the algorithms have been registered
    """
    def create_fake_function(files):
        def fake_function():
            pass
        for pyfile in files:
            if pyfile.endswith(".py"):
                name = pyfile.strip(".py")
                fake_function.__name__ = name
                if name not in globals():
                    globals()[name] = fake_function
    #
    directories = mtd.getConfigProperty("pythonalgorithms.directories").split(';')
    for top in directories:
        for root, dirs, files in os.walk(top):
            create_fake_function(files)

translate()
fake_python_alg_functions()