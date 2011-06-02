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
  
def create_algorithm(algorithm, version):
    """
        Create a function that will set up and execute an algorithm.
        The help that will be displayed is that of the most recent version.
        @param algorithm: name of the algorithm
    """
    
    def algorithm_wrapper(*args, **kwargs):
        """
            Note that if the Version parameter is passed, we will create
            the proper version of the algorithm without failing.
        """
        _version = version
        if "Version" in kwargs:
            _version = kwargs["Version"]
            kwargs.pop(kwargs.index("Version"))
        algm = mtd.createAlgorithm(algorithm, _version)
        algm.setPropertyValues(*args, **kwargs)
        algm.execute()
        return algm
    
    algorithm_wrapper.__name__ = algorithm
    algorithm_wrapper.__doc__ = mtd.createAlgorithmDocs(algorithm)
    
    # Dark magic to get the correct function signature
    # Calling help(...) on the wrapper function will produce a function 
    # signature along the lines of AlgorithmName(*args, **kwargs).
    # We will replace the name "args" by the list of properties, and
    # the name "kwargs" by "Version=1".
    #   1- Get the algorithm properties and build a string to list them,
    #      taking care of giving no default values to mandatory parameters
    _algm_object = mtd.createUnmanagedAlgorithm(algorithm, version)
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
    arg_str = ', '.join(arg_list)
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
    
    return algorithm_wrapper
    
def create_algorithm_dialog(algorithm, version):
    """
        Create a function that will set up and execute an algorithm dialog.
        The help that will be displayed is that of the most recent version.
        @param algorithm: name of the algorithm
    """
    def algorithm_wrapper(*args, **kwargs):
        _version = version
        if "Version" in kwargs:
            _version = kwargs["Version"]
            kwargs.pop(kwargs.index("Version"))
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
    _algm_object = mtd.createUnmanagedAlgorithm(algorithm, version)
    arg_list = []
    for p in mtd._getPropertyOrder(_algm_object):
        arg_list.append("%s=None" % p)
    arg_str = ', '.join(arg_list)
    signature = "\b%s" % arg_str
    f = algorithm_wrapper.func_code
    c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, f.co_consts, f.co_names,\
       (signature, "\b\bMessage=\"\", Enable=\"\", Disable=\"\", Version=%d" % version), \
       f.co_filename, f.co_name, f.co_firstlineno, f.co_lnotab, f.co_freevars)
    algorithm_wrapper.func_code = c  
    
    return algorithm_wrapper

def translate():
    """
        Loop through the algorithms and register a function call 
        for each of them
    """
    for algorithm in mtd._getRegisteredAlgorithms():
        globals()[algorithm[0]] = create_algorithm(algorithm[0], max(algorithm[1]))
        globals()["%sDialog" % algorithm[0]] = create_algorithm_dialog(algorithm[0], max(algorithm[1]))
            
translate()


