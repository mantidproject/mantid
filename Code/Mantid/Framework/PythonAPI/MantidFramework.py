"""Access the Mantid Framework.
"""
import os
import platform
import sys
import types
import copy
import inspect
import opcode
import __builtin__
import __main__
try:
    import mantidplot
    import PyQt4.QtCore as qtcore
    HAVE_GUI = True
except:
    HAVE_GUI = False # Assume no gui

try:
    import numpy
    HAVE_NUMPY = True
except ImportError:
    HAVE_NUMPY = False

# A list of filenames to suppress warnings about API removal
SUPPRESS_API_WARN = set()

###############################################################################
# Define the api version
###############################################################################
def apiVersion():
    """Indicates that this is version 1
    of the API
    """
    return 1

# Check whether MANTIDPATH is defined. If so, append it to the PYTHONPATH.
if os.getenv("MANTIDPATH") is not None:
    sys.path.append(os.getenv("MANTIDPATH"))
else:
    framework_file = os.path.abspath(__file__)
    os.environ["MANTIDPATH"]=os.path.split(framework_file)[0] # use the directory

# --- Import the Mantid API ---
if os.name == 'nt':
    from MantidPythonAPI import *
    from MantidPythonAPI import _binary_op, _equals_op
else:
    # The libMantidPythonAPI module is essentially statically linked
    # to boost python. However, we need to ensure the Mantid libraries
    # are loaded with the the RTLD_GLOBAL flag so that the singleton
    # symbols are shared across the boundaries.
    # 
    # We also need to coexist with the new-style interface meaning that
    # the boost python registry must be kept private in each api
    # so that multiple converters are not defined. This means that 
    # we cannot just pass the RTLD_GLOBAL flag here as this will 
    # cause the registry to be shared with the new api if it is 
    # loaded on top of this one. The only solution is to cherry
    # pick the modules that are loaded with the RTLD symbol.
    #
    # Another nice issue is that the dl module is broken on 64-bit
    # systems for Python 2.4 and ctypes doesn't exist there yet!
    # All in all this meant a small custom module calling dlopen
    # ourselves was the easiest way 
    import libdlopen
    dlloader = libdlopen.loadlibrary
    import subprocess
    
    _bin = os.path.abspath(os.path.dirname(__file__))
    pythonlib = os.path.join(_bin,'libMantidPythonAPI.so')
    if not os.path.exists(pythonlib):
        _bin = os.environ['MANTIDPATH']
        pythonlib = os.path.join(_bin,'libMantidPythonAPI.so')
        if not os.path.exists(pythonlib):
            raise RuntimeError('Unable to find libMantidPythonAPI, cannot continue')
    
    def get_libpath(mainlib, dependency):
        if platform.system() == 'Linux':
            cmd = 'ldd %s | grep %s' % (mainlib, dependency)
            subp = subprocess.Popen(cmd,stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,shell=True)
            out = subp.communicate()[0]
            # ldd produces a string that always has 4 columns. The full path
            # is in the 3rd column
            libpath = out.split()[2]
        else:
            libpath = os.path.join(_bin, dependency + '.dylib')
        return libpath
        
    library_var = "LD_LIBRARY_PATH"
    if platform.system() == 'Darwin':
        library_var = 'DY' + library_var
    ldpath = os.environ.get(library_var, "")
    ldpath += ":" + _bin
    os.environ[library_var] = ldpath
    if platform.system() == 'Linux':
        # stdc++ has to be loaded first or exceptions don't get translated 
        # properly across bounadries
        # NeXus has to be loaded as well as there seems to be an issue with
        # the thread-local storage not being initialized properly unles
        # it is loaded before other libraries.
        dlloader(get_libpath(pythonlib, 'stdc++')) 
        dlloader(get_libpath(pythonlib, 'libNeXus.so'))
    dlloader(get_libpath(pythonlib, 'libMantidKernel'))
    dlloader(get_libpath(pythonlib, 'libMantidGeometry'))
    dlloader(get_libpath(pythonlib, 'libMantidAPI'))

    oldflags = sys.getdlopenflags()
    if platform.system() == "Darwin":
        try:
            import dl
            RTLD_LOCAL = dl.RTLD_LOCAL
            RTLD_NOW = dl.RTLD_NOW
        except ImportError:
            RTLD_LOCAL = 0x4
            RTLD_NOW = 0x2
        sys.setdlopenflags(RTLD_LOCAL|RTLD_NOW)
    from libMantidPythonAPI import *
    from libMantidPythonAPI import _binary_op, _equals_op
    sys.setdlopenflags(oldflags)
# --- End of library load ---


#-------------------------------------------------------------------------------
def _makeString(value):
    """Make a string out of a value such that the Mantid properties can understand it
    """
    if HAVE_NUMPY and isinstance(value, numpy.ndarray):
        value = list(value) # Temp until more complete solution available (#2340)
    if isinstance(value, list) or isinstance(value, cpp_list_dbl)  \
            or isinstance(value, cpp_list_int) or isinstance(value, cpp_list_long):
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

#-------------------------------------------------------------------------------
def _decompile(code_object):
    """
    Taken from 
    http://thermalnoise.wordpress.com/2007/12/30/exploring-python-bytecode/

    Extracts dissasembly information from the byte code and stores it in 
    a list for further use.
    
    Call signature(s):
        instructions=decompile(f.f_code)

    Required      arguments:
    =========     =====================================================================
    code_object   A bytecode object extracted with inspect.currentframe()
                  or any other mechanism that returns byte code.   
    
    Optional keyword arguments: NONE    

    Outputs:
    =========     =====================================================================
    instructions  a list of offsets, op_codes, names, arguments, argument_type, 
                  argument_value which can be deconstructed to find out various things
                  about a function call.

    Example:
    # Two frames back so that we get the callers' caller
    f = inspect.currentframe().f_back.f_back  
    i = f.f_lasti  # index of the last attempted instruction in byte code
    ins = decompile(f.f_code)
    """
    code = code_object.co_code
    variables = code_object.co_cellvars + code_object.co_freevars
    instructions = []
    n = len(code)
    i = 0
    e = 0
    while i < n:
        i_offset = i
        i_opcode = ord(code[i])
        i = i + 1
        if i_opcode >= opcode.HAVE_ARGUMENT:
            i_argument = ord(code[i]) + (ord(code[i+1]) << (4*2)) + e
            i = i + 2
            if i_opcode == opcode.EXTENDED_ARG:
                e = iarg << 16
            else:
                e = 0
            if i_opcode in opcode.hasconst:
                i_arg_value = repr(code_object.co_consts[i_argument])
                i_arg_type = 'CONSTANT'
            elif i_opcode in opcode.hasname:
                i_arg_value = code_object.co_names[i_argument]
                i_arg_type = 'GLOBAL VARIABLE'
            elif i_opcode in opcode.hasjrel:
                i_arg_value = repr(i + i_argument)
                i_arg_type = 'RELATIVE JUMP'
            elif i_opcode in opcode.haslocal:
                i_arg_value = code_object.co_varnames[i_argument]
                i_arg_type = 'LOCAL VARIABLE'
            elif i_opcode in opcode.hascompare:
                i_arg_value = opcode.cmp_op[i_argument]
                i_arg_type = 'COMPARE OPERATOR'
            elif i_opcode in opcode.hasfree:
                i_arg_value = variables[i_argument]
                i_arg_type = 'FREE VARIABLE'
            else:
                i_arg_value = i_argument
                i_arg_type = 'OTHER'
        else:
            i_argument = None
            i_arg_value = None
            i_arg_type = None
        instructions.append( (i_offset, i_opcode, opcode.opname[i_opcode], i_argument, i_arg_type, i_arg_value) )
    return instructions

#-------------------------------------------------------------------------------

# A must list all of the operators that behave like a function calls in byte-code
# This is for the lhs functionality
__operator_names=set(['CALL_FUNCTION','UNARY_POSITIVE','UNARY_NEGATIVE','UNARY_NOT',
                      'UNARY_CONVERT','UNARY_INVERT','GET_ITER', 'BINARY_POWER',
                      'BINARY_MULTIPLY','BINARY_DIVIDE', 'BINARY_FLOOR_DIVIDE', 
                      'BINARY_TRUE_DIVIDE', 'BINARY_MODULO','BINARY_ADD','BINARY_SUBTRACT',
                      'BINARY_SUBSCR','BINARY_LSHIFT','BINARY_RSHIFT','BINARY_AND',
                      'BINARY_XOR','BINARY_OR', 'INPLACE_POWER', 'INPLACE_MULTIPLY', 
                      'INPLACE_DIVIDE', 'INPLACE_TRUE_DIVIDE','INPLACE_FLOOR_DIVIDE',
                      'INPLACE_MODULO', 'INPLACE_ADD', 'INPLACE_SUBTRACT', 
                      'INPLACE_LSHIFT','INPLACE_RSHIFT','INPLACE_AND', 'INPLACE_XOR',
                      'INPLACE_OR',
                      'COMPARE_OP'])

#-------------------------------------------------------------------------------

def lhs_info(output_type='both'):
    """Returns the number of arguments on the left of assignment along 
    with the names of the variables.

    Acknowledgements: 
       Thanks to Tim Charlton and Jon Taylor of the ISIS facility for 
       figuring this out.
     
    Call signature(s)::

    Required arguments: NONE
    
    Optional keyword arguments    Meaning:
    ===========================   ==========
    output_type                   A string enumerating the type of output, one of
                                    output_type = 'nreturns' : The number of return values
                                                      expected from the call
                                    output_type = 'names' : Just return a list of 
                                                      variable names 
                                    output_type = 'both' : A tuple containing both of
                                                      the above

    Outputs:
    =========     
    Depends on the value of the argument. See above.

    """
    # Two frames back so that we get the callers' caller, i.e. this should only
    # be called from within a function
    try:
        frame = inspect.currentframe().f_back.f_back
    except AttributeError:
        raise RuntimeError("lhs_info cannot be used on the command line, only within a function")

    # Process the frame noting the advice here:
    # http://docs.python.org/library/inspect.html#the-interpreter-stack
    try:
        ret_vals = _process_frame(frame)
    finally:
        del frame
        
    if output_type == 'nreturns':
        ret_vals = ret_vals[0]
    elif output_type == 'names':
        ret_vals = ret_vals[1]
    else:
        pass

    return ret_vals

#-------------------------------------------------------------------------------

def _process_frame(frame):
    """Returns the number of arguments on the left of assignment along 
    with the names of the variables for the given frame.
     
    Call signature(s)::

    Required arguments:
    ===========================   ==========
    frame                         The code frame to analyse

    Outputs:
    =========     
    Returns the a tuple with the number of arguments and their names
    """
    # Index of the last attempted instruction in byte code
    last_i = frame.f_lasti  
    ins_stack = _decompile(frame.f_code)

    call_function_locs = {}
    start_index = 0 
    start_offset = 0
    
    for index, instruction in enumerate(ins_stack):
        (offset, op, name, argument, argtype, argvalue) = instruction
        if name in __operator_names:
            call_function_locs[start_offset] = (start_index, index)
            start_index = index
            start_offset = offset

    (offset, op, name, argument, argtype, argvalue) = ins_stack[-1]
    # Append the index of the last entry to form the last boundary
    call_function_locs[start_offset] = (start_index, len(ins_stack)-1) 

    # In our case last_i should always be the offset of a call_function_locs instruction. 
    # We use this to bracket the bit which we are interested in
    output_var_names = []
    max_returns = []
    last_func_offset = call_function_locs[last_i][0]
    (offset, op, name, argument, argtype, argvalue) = ins_stack[last_func_offset + 1] 
    if name == 'POP_TOP':  # no return values
        pass
    if name == 'STORE_FAST' or name == 'STORE_NAME': # one return value 
        output_var_names.append(argvalue)
    if name == 'UNPACK_SEQUENCE': # Many Return Values, One equal sign 
        for index in range(argvalue):
            (offset_, op_, name_, argument_, argtype_, argvalue_) = ins_stack[last_func_offset + 2 +index] 
            output_var_names.append(argvalue_)
    max_returns = len(output_var_names)
    if name == 'DUP_TOP': # Many Return Values, Many equal signs
        # The output here should be a multi-dim list which mimics the variable unpacking sequence.
        # For instance a,b=c,d=f() => [ ['a','b'] , ['c','d'] ]
        #              a,b=c=d=f() => [ ['a','b'] , 'c','d' ]  So on and so forth.

        # put this in a loop and stack the results in an array.
        count = 0
        max_returns = 0 # Must count the max_returns ourselves in this case
        while count < len(ins_stack[call_function_locs[i][0]:call_function_locs[i][1]]):
            (offset_, op_, name_, argument_, argtype_, argvalue_) = ins[call_function_locs[i][0]+count] 
            if name_ == 'UNPACK_SEQUENCE': # Many Return Values, One equal sign 
                hold = []
                if argvalue_ > max_returns:
                    max_returns = argvalue_
                for index in range(argvalue_):
                    (_offset_, _op_, _name_, _argument_, _argtype_, _argvalue_) = ins[call_function_locs[i][0] + count+1+index] 
                    hold.append(_argvalue_)
                count = count + argvalue_
                output_var_names.append(hold)
            # Need to now skip the entries we just appended with the for loop.
            if name_ == 'STORE_FAST' or name_ == 'STORE_NAME': # One Return Value 
                if 1 > max_returns:
                    max_returns = 1
                output_var_names.append(argvalue_)
            count = count + 1

    return (max_returns,output_var_names)

#-------------------------------------------------------------------------------

def in_callstack(fn_name, frame):
    """Check whether the call stack from the given frame contains the given 
    function name.
    
    @param fn_name :: The function name to search for
    @param frame :: Start the search at this frame
    @returns True if the function name exists in the stack, false otherwise
    """
    if frame.f_code.co_name == fn_name: 
        return True
    while True:
        if frame.f_back:
            if frame.f_back.f_code.co_name == fn_name: 
                return True
            frame = frame.f_back
        else:
            break
    if frame.f_code.co_name == fn_name: 
        return True
    else:
        return False
    
    
#-------------------------------------------------------------------------------
def find_parent_python_algorithm(frame):
    """ By inspecting the call stack looking for PyExec(), find the
    parent PythonAlgorithm that is calling the current method.
    
    Returns
    -------
       The 'self' python algorithm object that is running PyExec().
       or, None if not found.
    """
    
    # We are looking for this method name, signifying PythonAlgorithm
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
    
#-------------------------------------------------------------------------------
def mtdGlobalHelp():
    # first part is algorithm name, second is version
    orig_algs = mtd._getRegisteredAlgorithms(include_hidden=False)

    # do the final formatting
    algs = []
    for alg in orig_algs:
        (name, version) = alg
        version = ["v%d" % it for it in version]
        version = " ".join(version)
        if version == "v1":
            algs.append(name)
        else:
            algs.append("%s %s" % (name, version))
    algs.sort()

    # print out the global help information
    print "The algorithms available are:\n"
    for alg in algs:
        print "  %s" % alg
    print "For help with a specific command type: help('cmd')"
    if HAVE_GUI:
        print "Note: Each command also has a counterpart with the word 'Dialog' appended ",
        print "to it, which when run will bring up a property input dialog for that algorithm." 

# Commit on behalf of Pete Peterson:        
def mantidHelp(cmd = None):
  if cmd == None or cmd == '':
    mtdGlobalHelp()
    return
  help(cmd)
        
## This function should generically go away, but for now...
#def mantidHelp(cmd = None):
#  if cmd == None or cmd == '':
#    mtdGlobalHelp()
#    return
#  try:
#    cmd = cmd.func_name
#  except AttributeError:
#    pass
#  try:
#    # Try exact case first as it will be quicker
#    exec('help(%s)' % cmd)
#    return
#  except NameError:
#    alg_name = mtd.isAlgorithmName(cmd)
#  if alg_name == '':
#    print 'mtdHelp(): "%s" not found in help list' % cmd
#  else:
#    exec('help(%s)' % alg_name)

# Some case variations on the mantidHelp function
mantidhelp = mantidHelp
mantidHelp = mantidHelp
MantidHelp = mantidHelp
mtdhelp = mantidHelp
mtdHelp = mantidHelp
Mtdhelp = mantidHelp
MtdHelp = mantidHelp

#-------------------------------------------------------------------------------
class ProxyObject(object):
    """Base class for all objects acting as proxys
    """
    def __init__(self, toproxy):
        self.__obj = toproxy
        
    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        return getattr(self._getHeldObject(), attr)

    def __str__(self):
        """
        Return a string representation of the proxied object
        """
        return str(self._getHeldObject())

    def __repr__(self):
        """
        Return a string representation of the proxied object
        """
        return `self._getHeldObject()`

    def _getHeldObject(self):
        """
        Returns a reference to the held object
        """
        return self.__obj

    def _kill_object(self):
        """
        Release the stored instance
        """
        self.__obj = None

    def _swap(self, obj):
        """
        Swap an object so that the proxy now refers to this object
        """
        self.__obj = obj



    
# Prefix for temporary objects within workspace binary operations
_binary_op_prefix = '__binary_tmp'
# A list of temporary workspaces created by algebraic operations
_binary_op_tmps = []

class WorkspaceProxy(ProxyObject):
    """
    A proxy object that stores a workspace instance. When the workspace is deleted
    from the ADS in Mantid, the object reference held here is set to 'None'
    """

    def __init__(self, obj, factory):
        """
        Create a proxy for the given object
        """
        super(WorkspaceProxy, self).__init__(obj)
        self.__factory = factory
        
    def __iter__(self):
        """
        Pass on to iterator if this is a table
        """
        if hasattr(self._getHeldObject(), '__iter__'):
            return self._getHeldObject().__iter__()
        else:
            raise AttributeError('Object does not support iteration')

    def __getitem__(self, index):
        """
        If we are a group then return a member, else return self
        """
        if self.isGroup():
            return self.__factory.create(self._getHeldObject().getNames()[index])
        else:
            raise AttributeError('Index invalid, object is not a group.')

    def __do_operation(self, op, rhs, inplace, reverse, lhs_vars):
        """
        Perform the given binary operation

        lhs_vars is expected to be a tuple containing the number of lhs variables and
        their names as the first and second element respectively
        """
        global _binary_op_tmps

        if lhs_vars[0] > 0:
            # Assume the first and clear the tempoaries as this
            # must be the final assignment
            if inplace:
                output_name = self.getName()
            else:
                output_name = lhs_vars[1][0]
            clear_tmps = True
        else:
            # Give it a temporary name and keep track of it
            clear_tmps = False
            output_name = _binary_op_prefix + str(len(_binary_op_tmps))
            _binary_op_tmps.append(output_name)

        # Do the operation
        if isinstance(rhs, WorkspaceProxy):
            rhs = rhs._getHeldObject()
        resultws = _binary_op(self._getHeldObject(),rhs, op, output_name, inplace, reverse)

        if clear_tmps:
            for name in _binary_op_tmps:
                if mtd.workspaceExists(name) and output_name != name:
                    mtd.deleteWorkspace(name)
            _binary_op_tmps = []
            
        if inplace:
            return self
        else:
            return self.__factory.create(resultws)

    def __add__(self, rhs):
        """
        Sum the proxied objects and return a new proxy managing that object
        """
        # Figure out the name of the output. Only the final function call that is
        # before the assignment will have the name of the variable
        lhs = lhs_info()
        return self.__do_operation('Plus', rhs,inplace=False, reverse=False,
                                   lhs_vars=lhs)

    def __radd__(self, rhs):
        """
        Sum the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Plus', rhs,inplace=False, reverse=True,
                                   lhs_vars=lhs)
    
    def __iadd__(self, rhs):
        """
        In-place sum the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Plus', rhs,inplace=True, reverse=False,
                                   lhs_vars=lhs)

    def __sub__(self, rhs):
        """
        Subtract the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Minus', rhs,inplace=False, reverse=False,
                                   lhs_vars=lhs)

    def __rsub__(self, rhs):
        """
        Handle a (double - workspace)
        """
        lhs = lhs_info()
        return self.__do_operation('Minus', rhs,inplace=False, reverse=True,
                                   lhs_vars=lhs)

    def __isub__(self, rhs):
        """
        In-place subtract the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Minus', rhs,inplace=True, reverse=False,
                                   lhs_vars=lhs)

    def __mul__(self, rhs):
        """
        Multiply the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Multiply', rhs,inplace=False, reverse=False,
                                   lhs_vars=lhs)

    def __rmul__(self, rhs):
        """
        Multiply the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Multiply', rhs,inplace=False, reverse=True,
                                   lhs_vars=lhs)

    def __imul__(self, rhs):
        """
        In-place multiply the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Multiply', rhs,inplace=True, reverse=False,
                                   lhs_vars=lhs)

    def __div__(self, rhs):
        """
        Divide the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Divide', rhs,inplace=False, reverse=False,
                                   lhs_vars=lhs)

    def __rdiv__(self, rhs):
        """
        Handle a double/workspace
        """
        lhs = lhs_info()
        return self.__do_operation('Divide', rhs,inplace=False, reverse=True,
                                   lhs_vars=lhs)

    def __idiv__(self, rhs):
        """
        In-place divide the proxied objects and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Divide', rhs,inplace=True, reverse=False,
                                   lhs_vars=lhs)
    
    def __lt__(self, rhs):
        """
        Do the (self < rhs) comparison and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('LessThan', rhs, inplace=False, reverse=False,
                                   lhs_vars=lhs)
    
    def __gt__(self, rhs):
        """
        Do the (self > rhs) comparison and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('GreaterThan', rhs, inplace=False, reverse=False,
                                   lhs_vars=lhs)
    
    def __or__(self, rhs):
        """
        Do the (self || rhs) comparison and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Or', rhs, inplace=False, reverse=False,
                                   lhs_vars=lhs)
    
    def __and__(self, rhs):
        """
        Do the (self && rhs) comparison and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('And', rhs, inplace=False, reverse=False,
                                   lhs_vars=lhs)
    
    def __xor__(self, rhs):
        """
        Do the (self ^ rhs) comparison and return a new proxy managing that object
        """
        lhs = lhs_info()
        return self.__do_operation('Xor', rhs, inplace=False, reverse=False,
                                   lhs_vars=lhs)
        

    def __pow__(self, y):
        """ Raise a workspace to a power. Equivalent of x**y.
         
        Args:
            x :: workspace or other type.
            y :: exponent
        """
        lhs = lhs_info()
        return self.__do_unary_operation("PowerMD", lhs, Exponent=y)
        

    def __do_unary_operation(self, op, lhs_vars, **kwargs):
        """
        Perform the unary operation

        Args:
            op :: name of the algorithm to run
            lhs_vars :: is expected to be a tuple containing the number of lhs variables and
                their names as the first and second element respectively
            kwargs :: additional properties to give to algorithm
        """
        global _binary_op_tmps

        if lhs_vars[0] > 0:
            # Assume the first and clear the tempoaries as this
            # must be the final assignment
            output_name = lhs_vars[1][0]
            clear_tmps = True
        else:
            # Give it a temporary name and keep track of it
            clear_tmps = False
            output_name = _binary_op_prefix + str(len(_binary_op_tmps))
            _binary_op_tmps.append(output_name)

        # Do the operation
        alg = mtd.createAlgorithm(op)
        alg.setPropertyValue("InputWorkspace", self.getName())
        alg.setPropertyValue("OutputWorkspace", output_name)
        for (key,value) in kwargs.items():
            alg.setPropertyValue(key,str(value))
        alg.execute()
        resultws = alg.workspace()

        if clear_tmps:
            for name in _binary_op_tmps:
                if mtd.workspaceExists(name) and output_name != name:
                    mtd.deleteWorkspace(name)
            _binary_op_tmps = []
            
        return resultws
        
    def __invert__(self):
        """
        Return the inversion (NOT operator) on self
        """
        lhs = lhs_info()
        return self.__do_unary_operation('NotMD', lhs_vars=lhs)

    
    def equals(self, rhs, tol):
        """
        Checks whether the given workspace matches this one within the allowed
        tolerance. 
            rhs - The workspace to compare with
            tol - A tolerance value, e.g. 1e-08
            
        Returns True if the workspaces contain the same data, false otherwise
        """
        return _equals_op(self._getHeldObject(), rhs._getHeldObject(), tol)

    def isGroup(self):
        """
        Is the data object a WorkspaceGroup or not
        """
        if isinstance(self._getHeldObject(), WorkspaceGroup):
            return True
        else:
            return False

    def getRun(self):
        """
        Return an object describing properties of the run
        """
        return RunProxy(self._getHeldObject().getRun())

#-------------------------------------------------------------------------------
class RunProxy(ProxyObject):
    """Holds information regarding the run as a list of properties.

       It defines a dictionary interface for the run properties.
    """
    def __init__(self, runobj):
        """
        Constructor
        """
        super(RunProxy, self).__init__(runobj)
        self.__properties = {}
        props = runobj.getProperties()
        for prop in props:
            self.__properties[prop.name] = prop

    def keys(self):
        """
        Return a list of property names
        """
        return self.__properties.keys()

    def __contains__(self, key):
        """
        Does the run contain a given property?
        Returns true if it does, false otherwise
        """
        return key in self.__properties

    def __getitem__(self, key):
        """
        Returns the value of the item with the given key.
        If the key is not contained within the property list
        then a KeyValueError is raised
        """
        return self.__properties[key]

    def get(self, key, default=None):
        """
        Returns the value of the item with the given key or default if
        the key does not exist
        """
        if key in self.__properties:
            return self[key]
        else:
            return default

    def __setitem__(self, key, value, overwrite=False):
        """
        Sets the value of the item with the given key. This has limited 
        support for types.
        """
        print "__setitem__(%s, %s, %s, %s)" % (self, key, value, overwrite)
        value_type = type(value)
        runobj = self._getHeldObject()
        if value_type == type(1.):
            runobj.addProperty_dbl(key, value, overwrite)
        elif value_type == type(1):
            runobj.addProperty_int(key, value, overwrite)
        elif value_type == type(""):
            runobj.addProperty_str(key, value, overwrite)
        else:
            raise TypeError("Not working for %s" % str(type(value)))
        prop = self._getHeldObject().getProperty(key)
        self.__properties[key] = prop


#-------------------------------------------------------------------------------

class WorkspaceProxyFactory(object):
    
    def __init__(self, garbage_collector, framework):
        self.__gc = garbage_collector
        self.__framework = framework
                                         
    def create(self, obj):
        wksp = obj
        if isinstance(obj, str):
            wksp = self.__framework._retrieveWorkspace(obj)
        if wksp == None:
            return None
        proxy = WorkspaceProxy(wksp, self)
        self.__gc.register(wksp.getName(), proxy)
        return proxy

#-------------------------------------------------------------------------------

class WorkspaceGarbageCollector(object):
    """
    A register of workspaces that have been retrieved from Mantid
    """
    
    def __init__(self):
        self._refs = {}

    def register(self, name, proxy):
        """
        Register a name and reference to the store
        """
        self._refs[name] = proxy

    def replace(self, name, wksp):
        """
        Replace an object reference within a proxy
        """
        try:
            self._refs[name]._swap(wksp)
        except Exception:
            pass

    def kill_object(self, name):
        """
        Signal the proxy to nullify its stored reference
        """
        try:
            self._refs[name]._kill_object()
            # Remove the key as we don't want to keep the reference around
            del self._refs[name]
        except Exception:
            pass

    def kill_all(self):
        """
        Kill all references to data objects
        """
        # Note here that a simple clear won't do as it won't set the object reference to None 
        for w in self._refs:
            self._refs[w]._kill_object()
        self._refs.clear()
#---------------------------------------------------------------------------------------

class IAlgorithmProxy(ProxyObject):
    """
    A proxy object for IAlgorithm returns
    """

    def __init__(self, ialg, framework):
        super(IAlgorithmProxy, self).__init__(ialg)
        self.__framework = framework
        self.__havelist = False
        self.__wkspnames = []
        self.__propertynames = {} # the names of properties and their directions
                                  # 0 - input
                                  # 1 - output
        for prop in ialg.getProperties():
            self.__propertynames[prop.name] = prop.direction
        self.__propertyOrder = None
        
    def workspace(self):
        return self._retrieveWorkspaceByIndex(0)

    def keys(self):
        return self.__propertynames.keys()

    def __contains__(self, key):
        if key in self.__propertynames.keys():
            return True
        # TODO should there be more checks?
        return False

    def __getitem__(self, key):
        if key in self.__propertynames.keys():
            # get the value of the properties
            value = self._getHeldObject().getPropertyValue(key)

            # convert to a workspace if appropriate
            self._createWorkspaceList()
            if value in self.__wkspnames:
                return self.__framework[value]
            else:
                return self._getHeldObject().getProperty(key)
        else:
            return self._retrieveWorkspaceByIndex(key)

    def _retrieveWorkspaceByIndex(self, index):
        self._createWorkspaceList()

        if len(self.__wkspnames) > 0:
            return self.__framework[self.__wkspnames[int(index)]]
        else:
            raise RuntimeError("'%s' has no output workspaces." % self._getHeldObject().name())

    def _createWorkspaceList(self):
        if self.__havelist == True:
            return

        # Create a list for the output workspaces
        props = self._getHeldObject().getProperties()
        for p in props:
            if p.direction != 1:
                continue
            if issubclass(type(p), IWorkspaceProperty):
               self.__wkspnames.append(p.value)
        self.__havelist = True

    def __setProperty(self, name, value, convertToString=False):
        """
        Friendly error checking version of setProperty that calls the right version of everything.
        
        @param name: Name of the property to set.
        @param value: The value of the property.
        @param param: Coerce everyhing into a string if True.   
        """
        ialg = self._getHeldObject()
        # proactively look for the property to exist
        if not ialg.existsProperty(name):
            msg = 'Unknown property "%s" for %s version %d' % (name, ialg.name(), ialg.version())
            raise AttributeError(msg)

        if value is None:
            return
        if convertToString:
            value = _makeString(value).lstrip('? ')

        try:
            if isinstance(value, WorkspaceProxy):
                ialg._setWorkspaceProperty(name, value._getHeldObject())
            elif isinstance(value, str):
                ialg.setPropertyValue(name, value)
            else:
                ialg.setPropertyValue(name, _makeString(value).lstrip('? '))
        except RuntimeError, exc: # Confirms what kind of error just happened and rethrows
            if str(exc).startswith('Unknown property search object'): # shouldn't happen
                msg = 'Unknown property "%s" for %s version %d' % (name, ialg.name(), ialg.version())
                raise AttributeError(msg)
            else:
                raise  # rethrow the error
        
    def setPropertyValues(self, *args, **kwargs):
        """
        Set all of the properties of the algorithm. Everything gets converted into a string.
        One should generally try to use setProperties instead.
        """
        if self.__propertyOrder is None:
            self.__propertyOrder = mtd._getPropertyOrder(self._getHeldObject())

        # add the args to the kw list so everything can be set in a single way
        for (key, arg) in zip(self.__propertyOrder[:len(args)], args):
            kwargs[key] = arg

        # set the properties of the algorithm
        ialg = self._getHeldObject()
        for key in kwargs.keys():
            self.__setProperty(key, kwargs[key], True)
        
    def setProperties(self, *args, **kwargs):
        """
        Set all of the properties of the algorithm. Everything except Workspaces get converted into a string.
        """
        if self.__propertyOrder is None:
            self.__propertyOrder = mtd._getPropertyOrder(self._getHeldObject())

        # add the args to the kw list so everything can be set in a single way
        for (key, arg) in zip(self.__propertyOrder[:len(args)], args):
            kwargs[key] = arg

        # set the properties of the algorithm
        for key in kwargs.keys():
            self.__setProperty(key, kwargs[key], False)

    def setPropertiesDialog(self, *args, **kwargs):
        """
        Set the properites all in one go assuming that you are preparing for a
        dialog box call. If the dialog is canceled do a sys.exit, otherwise 
        return the algorithm ready to execute.
        """
        if not HAVE_GUI:
            raise RuntimeError("Can only display properties dialog in gui mode")

        # generic setup
        enabled_list = [s.lstrip(' ') for s in kwargs.get("Enable", "").split(',')]
        del kwargs["Enable"] # no longer needed
        disabled_list = [s.lstrip(' ') for s in kwargs.get("Disable", "").split(',')]
        del kwargs["Disable"] # no longer needed
        message = kwargs.get("Message", "")
        del kwargs["Message"]
        presets = '|'
        # configure everything for the dialog
        for name in kwargs.keys():
            value = kwargs[name]
            if value is not None:
                presets += name + '=' + _makeString(value) + '|'

        # finally run the configured dialog
        dialog = mantidplot.createPropertyInputDialog(self.name(), presets, message, enabled_list, disabled_list)
        if dialog == False:
            sys.exit('Dialog cancel pressed. Script execution halted.')

#---------------------------------------------------------------------------------------

class MantidPyFramework(FrameworkManager):
    """
    The Mantid framework object.
    
    Provides access to Mantid.
    """
    #### special methods ######################################################
    
    __is_initialized = False
    __config_service = None
    
    def __init__(self):
        # Call base class constructor
        super(MantidPyFramework, self).__init__()
        self._garbage_collector = WorkspaceGarbageCollector()
        self._proxyfactory = WorkspaceProxyFactory(self._garbage_collector, self)

    def runtime_mtdsimple(self, is_runtime=True):
        self.sendLogMessage("DEPRECATION WARNING: mtd.runtime_mtdsimple() is no longer used. Simply remove this line from your code.")

    def __getitem__(self, key):
        """
        Enables the framework to be used in a dictionary like manner.
        It returns the MatrixWorkspace proxy for the given name
        """
        return self._proxyfactory.create(self._retrieveWorkspace(key))

    def __iter__(self):
        """Returns an iterator object that can loop through the workspaces.

        Enables:

            for w in mantid:
                print w.getNumberHistograms()
        """
        # Implemented using the generator pattern
        # http://docs.python.org/tutorial/classes.html#generators
        ws_names = self.getWorkspaceNames()
        for name in ws_names:
            yield mtd[name]    

    def __contains__(self, key):
        """Returns true if the key is a known workspace in the ADS
        """
        return mtd.workspaceExists(key)

    ###########################################################################
    # Framework interface
    ###########################################################################
        
    #### properties ###########################################################
            
    def getSettings(self):
        """
        Returns an object that accesses Mantid's configuration settings
        """
        if self.__config_service is None:
            self.__config_service = ConfigService()
        return self.__config_service
        
    settings = property(getSettings)
    
    #### methods ###########################################################

    def initialise(self, gui=None):
        """
        Initialise the framework
        """
        if self.__is_initialized == True:
            return

        if gui is None:
            self.__gui__ = HAVE_GUI
        else:
            self.__gui__ = gui

        # Run through init steps 
        import mantidsimple as _mantidsimple
        dir_list = mtd.getConfigProperty("python.plugins.directories").split(';')
        dir_list += mtd.getConfigProperty("user.python.plugins.directories").split(';')
        
        _mantidsimple.mockup(dir_list)
        pyalg_loader = PyAlgLoader()
        plugins = pyalg_loader.load_modules(dir_list, refresh=False)
        new_attrs = _mantidsimple.translate() # Make sure the PythonAlgorithm functions are written
        _sync_attrs(_mantidsimple, new_attrs,plugins)
        
        self.__is_initialized = True

    # Overload for the 'other' spelling
    def initialize(self, GUI=None):
        self.initialise(GUI)

    def list(self):
        """
        Print a list of the workspaces stored in Mantid along with their type
        """
        names = self.getWorkspaceNames()
        n_names = names.size()
        output = []
        max_width = 0
        for i in range(0, n_names):
            wksp = self._retrieveWorkspace(names[i])
            output.append([names[i],''])
            max_width = max(max_width, len(names[i]))
            if( isinstance(wksp, MatrixWorkspace) ):
                output[i][1] = 'MatrixWorkspace'
            elif( isinstance(wksp, ITableWorkspace) ):
                output[i][1] = 'TableWorkspace'
            else:
                output[i][1] = 'WorkspaceGroup'
        
        max_width += 3
        output_table = 'Mantid currently holds the following workspaces:\n'
        for row in output:
            output_table += '\t' + row[0].ljust(max_width, ' ') + '-   ' + row[1] + '\n'

        print output_table

    def keys(self):
        """
        Returns a list of the workspace names contained within Mantid, i.e.
        workspaces = mtd.keys()

            for i in workspaces:
              w = mtd[i]  # Handle to workspace
              ...

        would allow looping over all of the available workspaces.
        """
        ws_names = self.getWorkspaceNames()
        key_list = []
        for name in ws_names:
            key_list.append(name)
        return key_list

    def createAlgorithm(self, name, version=-1):
        """Creates an algorithm object. The object
        is then wrapped in an AlgorithmProxy and returned.
        This allows things such as alg.workspace() to be 
        used.

        @see createManagedAlgorithm for creating an algorithm
        object alone
        """
        if isinstance(name, IAlgorithmProxy): # it already is the right thing
            return name
        else:
            return self._createAlgProxy(name, version)

    def _createAlgProxy(self, ialg, version=-1):
        """
        Will accept either a IAlgorithm or a string specifying the algorithm name.
        """
        if isinstance(ialg, IAlgorithmProxy):
            return ialg
        if isinstance(ialg, str):
            # Look for a parent python algorithm
            parentAlg = find_parent_python_algorithm(inspect.currentframe())
            if not parentAlg is None:
                
                # Algorithm is being called as part of a Python algorithm.
                ialg = parentAlg._createChildAlgorithm(ialg, version)

                # History is always active until the workflow algorithms are in place
                ialg.enableHistoryRecordingForChild(True)
                
                # Children do not log if the parent is not logging
                ialg.setLogging( parentAlg.isLogging() )
                
                # Most python algorithms use the NAMES of workspace, so each child
                # will store its workspaces in the ADS.
                ialg.setAlwaysStoreInADS(True)
            else:
                ialg = self.createManagedAlgorithm(ialg, version) 
        ialg.setRethrows(True) # Ensure the console rethrows.
        return IAlgorithmProxy(ialg, self)

    # make what comes out of C++ a little friendlier to use
    def _getRegisteredAlgorithms(self, include_hidden = False):
        # get the full list from C++
        algs = super(MantidPyFramework, self)._getRegisteredAlgorithms(include_hidden)
        # split the string into name and version
        algs = [item.split('|') for item in algs]
        # convert the version into an integer
        algs = [(item[0], int(item[1])) for item in algs]

        # convert the list into a dict
        algs_dict = {}
        for alg in algs:
            (name, version) = alg
            version = [version]
            if algs_dict.has_key(name):
                version.extend(algs_dict[name])
            version.sort()
            algs_dict[name] = version

        algs = []
        names = algs_dict.keys()
        names.sort()
        for name in names:
            algs.append((name, tuple(algs_dict[name])))

        return algs

    #### private methods ###########################################################
    
    @staticmethod
    def _addToPySearchPath(dirs):
        """
        Add the given directories to Python's search path
        """
        if type(dirs) == str:
            if dirs.rstrip() == "": return
            dirs = dirs.split(';')
        if type(dirs) != list:
            return
        for path in dirs:
            if path.endswith('/') or path.endswith("\\"):
                path = os.path.dirname(path)
            if not path in sys.path:
                sys.path.append(path)

    def _refreshPyAlgorithms(self):
        # Reload the modules that have been loaded
        self._pyalg_loader.load_modules(refresh=True)
                
    def _retrieveWorkspace(self, name):
        """
        Use the appropriate function to return the workspace that has that name
        """
        # 99% of the time people are using matrix workspaces but we still need to check
        
        # Try each workspace type in order, from more specialised to less specialised.
        attrs = ['_getRawIPeaksWorkspacePointer', '_getRawIEventWorkspacePointer', '_getRawMatrixWorkspacePointer',
                 '_getRawIMDEventWorkspacePointer','_getRawIMDHistoWorkspacePointer',
                '_getRawIMDWorkspacePointer', '_getRawWorkspaceGroupPointer', '_getRawTableWorkspacePointer']
        retval = None
        try:
            for att in attrs:
                retval = getattr(self, att)(name)
                if retval is not None: 
                    break
        except RuntimeError:
            pass
        # Preserve behaviour
        return retval

    def _workspaceRemoved(self, name):
        """
        Called when a workspace has been removed from the Mantid ADS
        """
        self._garbage_collector.kill_object(name);

    def _workspaceReplaced(self, name):
        """
        Called when a workspace has been removed from the Mantid ADS
        """
        wksp = self._retrieveWorkspace(name)
        if wksp != None:
            self._garbage_collector.replace(name, wksp)

    def _workspaceStoreCleared(self):
        """
        Called when the ADS is cleared
        """
        self._garbage_collector.kill_all()

    def _algorithmFactoryUpdated(self):
        """
        Called in reponse to an algorithm factory update
        """
        # Reload the simple api
        self._importSimpleAPIToMain()

      
# *** Legacy functions ***

    def getMatrixWorkspace(self, name):
        """
        Get a matrix workspace by name. Returns a proxy object
        """
        return self._proxyfactory.create(self._getRawMatrixWorkspacePointer(name))
    
    def getTableWorkspace(self, name):
        """
        Get a table workspace by name. Returns a proxy object
        """
        return self._proxyfactory.create(self._getRawTableWorkspacePointer(name))
        
    def getMatrixWorkspaceGroup(self, name):
        """
        Get a list of matrix workspaces
        """
        wksp_grp = self._getRawWorkspaceGroupPointer(name)
        # Build a list of proxy objects
        names = wksp_grp.getNames()
        return [ self.getMatrixWorkspace(w) for w in names[1:] ]


#-------------------------------------------------------------------------------------------
##
# Loader object for Python algorithms
##

class PyAlgLoader(object):

    __CHECKLINES__ = 100
    
    def load_modules(self, dir_list, refresh=False):
        """
        Import Python modules containing Python algorithms
        """
        if len(dir_list) == 0: 
            mtd.sendLogMessage('PyAlgLoader.load_modules: no python algorithm directory found')
            return
        # Check defined Python algorithm directories and load any modules
        changes = False
        loaded_modules = []
        for path in dir_list:
            if path == '':
                continue
            if not os.path.isdir(path):
                mtd.sendLogMessage(path + ' is not a directory' )
                continue
            changes, plugins = self._importAlgorithms(path, refresh)
            loaded_modules += plugins
        
        return loaded_modules

# ------- Private methods --------------
#
    def _importAlgorithms(self, path, refresh):
        # Make sure the directory doesn't contain a trailing slash
        path = path.rstrip("/").rstrip("\\")
        try:
            files = os.listdir(path)
        except(OSError):
            return False
        changes = False
        
        def _process_file(file_path, modname):
            pyext = '.py'
            if not modname.endswith(pyext):
                return
            original = os.path.join(file_path, modname)
            modname = modname[:-len(pyext)]
            compiled = os.path.join(file_path, modname + '.pyc')
            if modname in sys.modules and \
               os.path.exists(compiled) and \
               os.path.getmtime(compiled) >= os.path.getmtime(original):
                return
            try:               
                if self._containsOldAPIAlgorithm(original):
                    # Temporarily insert into path
                    sys.path.insert(0, file_path)
                    if modname in sys.modules:
                        reload(sys.modules[modname])
                    else:
                        __import__(modname)
                    changes = True
                    # Cleanup system path
                    del sys.path[0]
                    return sys.modules[modname]
            except(StandardError), exp:
                mtd.sendLogMessage('Error: Importing module "%s" failed". %s' % (modname,str(exp)))
            except:
                mtd.sendLogMessage('Error: Unknown error on Python algorithm module import. "%s" skipped' % modname)

        # Find sub-directories
        plugins = []
        for root, dirs, files in os.walk(path):
            for f in files:
                loaded_module = _process_file(root, f)
                if loaded_module is not None:
                    plugins.append(loaded_module)
            
        return changes, plugins

    def _containsOldAPIAlgorithm(self, modfilename):
        file = open(modfilename,'r')
        alg_found = True
        for line in reversed(file.readlines()):
            if 'registerAlgorithm' in line:
                alg_found = False
                break
        file.close()
        return alg_found
#-------------------------------------------------------------------------------------------
def _sync_attrs(source_module, attrs, clients):
    """
        Syncs the attribute definitions between the 
        given list from the source module & list of client modules such
        that the function defintions point to the same
        one
        
        @param source_module :: The module containing the "correct"
                                definitions
        @param attrs :: The list of attributes to change in the client modules
        @param clients :: A list of modules whose attribute definitions
                          should be taken from source
    """
    for func_name in attrs:
        attr = getattr(source_module, func_name)
        for plugin in clients:
            if hasattr(plugin, func_name):
                setattr(plugin, func_name, attr)

#-------------------------------------------------------------------------------------------

##
# PyAlgorithm class
##

class PythonAlgorithm(PyAlgorithmBase):
    """
    Base class for all Mantid Python algorithms
    """
    sequencer = None

    def __init__(self):
        super(PythonAlgorithm,self).__init__()
        # Dictionary of property names/types
        self._proptypes = {}
   
    def name(self):
        return self.__class__.__name__

    def version(self):
        return 1
        
    def category(self):
        return 'PythonAlgorithms'
    
    def PyInit(self):
        raise NotImplementedError('PyInit() method must be defined for a Python algorithm')

    def PyExec(self):
        raise NotImplementedError('PyExec() method must be defined for a Python algorithm')

    # declareProperty "wrapper" function so that we don't need separate named functions for each type
    def declareProperty(self, Name, DefaultValue, Validator = None, Description = '', Direction = Direction.Input):
        """
        Declare a property for this algorithm
        """
        # Test default value type and call the relevant function
        if isinstance(DefaultValue, float):
            decl_func = self.declareProperty_dbl
            ptype = float
        elif isinstance(DefaultValue, str):
            decl_func = self.declareProperty_str
            ptype = str
        elif isinstance(DefaultValue,bool):
            decl_func = self.declareProperty_bool
            ptype = bool
        elif isinstance(DefaultValue, int) or isinstance(DefaultValue, long):
            decl_func = self.declareProperty_int
            ptype = int
        else:
            raise TypeError('Unrecognized type for property "' + Name + '"')

        if Validator == None:
            decl_func(Name, DefaultValue, Description, Direction)
        else:
            decl_func(Name, DefaultValue, _createCPPValidator(ptype, Validator), Description, Direction)

        self._mapPropertyToType(Name, ptype)

    def declareListProperty(self, Name, DefaultValue, Validator = None, Description = '', Direction = Direction.Input):
        # If the value pass here is a type then given back an empty list and the correct C++ declareListProperty function
        # If the value is a list check that all of the elements have the same type and give back the C++ declareListProperty function
        if isinstance(DefaultValue,type):
            if DefaultValue == int:
                decl_func = self.declareListProperty_int
                ltype = int
            elif DefaultValue == float:
                decl_func = self.declareListProperty_dbl
                ltype = float
            elif DefaultValue == str:
                decl_func = self.declareListProperty_str
                ltype = str
            else:
                raise TypeError('Unrecognized list type requested for property "' + Name + '"')
            DefaultValue = []
        elif isinstance(DefaultValue, list):
            try:
                decl_func,ltype = self._getListType(DefaultValue)
            except TypeError, err:
                raise TypeError(str(err) + ' for property "' + Name + '"')
        else:
            raise TypeError('Unrecognized default value for list property ' + Name + "'")
            
        if Validator == None:
            decl_func(Name, DefaultValue, Description, Direction)
        else:
            decl_func(Name, DefaultValue, _createCPPValidator(ltype, Validator), Description, Direction)
            
        self._mapPropertyToType(Name, [list, ltype])
        
    # Specialized version for workspaces
    def declareWorkspaceProperty(self, PropertyName, WorkspaceName, Direction, Validator = None, \
                                     Description = '', Type = MatrixWorkspace, Optional=False):
        if Type == MatrixWorkspace:
            decl_fn = self._declareMatrixWorkspace
        elif Type == Workspace:
            decl_fn = self._declareWorkspace
        elif Type == ITableWorkspace:
            decl_fn = self._declareTableWorkspace
        else:
            raise TypeError('Unrecognized type of workspace specified for property "' + PropertyName + '"')

        if Validator == None:
            decl_fn(PropertyName, WorkspaceName, Description, Direction, Optional)
        else:
            decl_fn(PropertyName, WorkspaceName, Validator, Description, Direction, Optional)
            
        self._mapPropertyToType(PropertyName, WorkspaceProperty)

    # Specialized version for FileProperty
    def declareFileProperty(self, Name, DefaultValue, Type, Exts = [], Description = '',  Direction = Direction.Input):
        if not isinstance(DefaultValue, str):
            raise TypeError('Incorrect default value type for file property "' + Name + '"')
        try:
            self._declareFileProperty(Name, DefaultValue, Type, Exts, Description, Direction)
        except(TypeError):
            raise TypeError('Invalid type in file extension list for property "' + Name + '"')
        self._mapPropertyToType(Name, str)

    # Specialize version for algorithm property
    def declareAlgorithmProperty(self, name, description = '', direction = Direction.Input):
        self._declareAlgorithmProperty(name, description, direction)
        self._mapPropertyToType(name, AlgorithmProperty)
        
    # getProperty method  wrapper
    def getProperty(self, Name):
        """
        Retrieve a property value for the given name
        """
        try:
            prop_type = self._proptypes[Name]
        except KeyError:
            raise KeyError('Unknown property name "' + Name + '"')

        if prop_type == int:
            return self.getProperty_int(Name)
        elif prop_type == float:
            return self.getProperty_dbl(Name)
        elif prop_type == bool:
            return self.getProperty_bool(Name)
        elif prop_type == str:
            return self.getPropertyValue(Name)
        elif prop_type == AlgorithmProperty:
            return self._getAlgorithmProperty(Name)
        elif isinstance(prop_type,list):
            if prop_type[0] == list:
                ltype = prop_type[1]
                if ltype == int:
                    return self.getListProperty_int(Name)
                elif ltype == float:
                    return self.getListProperty_dbl(Name)
                elif ltype == str:
                    return self.getListProperty_str(Name)
                else:
                    raise TypeError('Cannot retrieve unrecognized list property "' + Name + '"')
        elif issubclass(prop_type, WorkspaceProperty):
            return mantid[self.getPropertyValue(Name)]
        else:
            raise TypeError('Cannot retrieve unrecognized type for property "' + Name + '"')

    # Wrapper around setPropertyValue
    def setProperty(self, Name, Value):
        try:
            prop_type = self._proptypes[Name]
        except KeyError:
            raise KeyError('Attempting to set unknown property "' + Name + '"')
        
        value_type = self._typeof(Value)
        if prop_type != value_type:
            raise TypeError("Property '" + Name + "' declared as '" + str(prop_type) + "' but set as '" + str(value_type) + "'")

        if value_type == str:
            self.setPropertyValue(Name, Value)
        elif value_type == bool:
            self.setPropertyValue(Name, str(int(Value)))
        elif value_type == AlgorithmProperty:
            if isinstance(Value, IAlgorithmProxy):
                Value = Value._getHeldObject()
            if type(Value).__name__=="IAlgorithm":
                self._setAlgorithmProperty(Name, Value)
            else:
                raise TypeError('Attempting to set an algorithm property with value that is not an algorithm')
        elif value_type == WorkspaceProperty:
            if isinstance(Value, WorkspaceProxy):
                Value = Value._getHeldObject()
            if isinstance(Value, MatrixWorkspace):
                self._setMatrixWorkspaceProperty(Name, Value)
            elif isinstance(Value, ITableWorkspace):
                self._setTableWorkspaceProperty(Name, Value)
            elif isinstance(Value, Workspace):
                # Generic workspace
                self._setWorkspaceProperty(Name, Value)
            else:
                raise TypeError('Attempting to set workspace property with value that is not a workspace.')
        else:
            self.setPropertyValue(Name, str(Value))
        
    # Return a type based on the value. This avoids the use of Python built-in type function since it
    # returns something that is too generic for user-define class types
    def _typeof(self, value):
        if isinstance(value, int) or isinstance(value, long):
            return int
        elif isinstance(value, float):
            return float
        elif isinstance(value, str):
            return str
        elif isinstance(value, bool):
            return bool
        elif isinstance(value,list):
            return list
        elif isinstance(value, IAlgorithmProxy) or type(value).__name__=="IAlgorithm":
            return AlgorithmProperty
        elif isinstance(value, WorkspaceProxy) or isinstance(value, MatrixWorkspace) or isinstance(value, ITableWorkspace):
            return WorkspaceProperty
        else:
            raise TypeError("Unknown property type for value '" + str(value) + "'")
                    
    # Execute a string that declares a property and keep track of the registered type
    def _mapPropertyToType(self, name, prop_type):
        self._proptypes[name] = prop_type

    def _getListType(self,values):
        """
        Return the declareList function relevant to the given list
        """
        ptype = type(values[0])
        for v in values:
            if type(v) != ptype:
                raise TypeError('List property contains mixed types')
    
        if ptype == int:
            return self.declareListProperty_int, int
        elif ptype == float:
            return self.declareListProperty_dbl, float
        elif ptype == str:
            return self.declareListProperty_str, str
        else:
            raise TypeError('Unrecognized list type')
        
    def _createChildAlgProxy(self, ialg, version=-1):
        """
        Will accept either a IAlgorithm or a string specifying the algorithm name.
        """
        if isinstance(ialg, str):
            ialg = self._createChildAlgorithm(str(ialg), version)
        ialg.setRethrows(True) # TODO get rid of this line # We NEED this line for Child Algorithms.
        return IAlgorithmProxy(ialg, self)
        
    def executeChildAlg(self, algorithm, *args, **kwargs):
        """
        Execute an algorithm as a Child Algorithm.
        The method will set the input/output workspace objects so that they can be
        passed to Child Algs without having to save them in the ADS.
        
        @param algorithm: mantidsimple algorithm function returning an IAlgorithm object
        @param *args: arguments to pass to the instantiated algorithm
        @param **kwargs: keyword arguments to pass to the instantiated algorithm
        @return: algorithm proxy for the executed algorithm
        """
        if isinstance(algorithm, str):
            proxy = self._createChildAlgProxy(algorithm)
        else:
            #TODO: check if we still need this after mantidsimple is gone
            if isinstance(algorithm, types.FunctionType):
                proxy = self._createChildAlgProxy(algorithm.func_name)
            else:
                raise RuntimeError, "PythonAlgorithm.executeChildAlg expects a function."

        if not isinstance(proxy, IAlgorithmProxy):
            raise RuntimeError, "PythonAlgorithm.executeChildAlg expects a function returning an IAlgorithm object"                
        
        proxy.setPropertyValues(*args, **kwargs)
        proxy.execute()
        return proxy

#------------------------------------------------------------------------------------------------
# WorkspaceFactory
class WorkspaceFactory(WorkspaceFactoryProxy):
    
    @staticmethod
    def createMatrixWorkspace(NVectors, XLength, YLength):
        return WorkspaceFactoryProxy.createMatrixWorkspace(NVectors, XLength, YLength)
    
    @staticmethod
    def createMatrixWorkspaceFromCopy(Copy, NVectors = -1, XLength = -1, YLength = -1):
        if isinstance(Copy, WorkspaceProxy):
            Copy = Copy._getHeldObject()
        if Copy == None:
            raise RuntimeError("Cannot create MatrixWorkspace from copy, original is None")
        if not isinstance(Copy,MatrixWorkspace):
            raise RuntimeError('Cannot create copy of MatrixWorkspace from template type given.')
        return WorkspaceFactoryProxy.createMatrixWorkspaceFromTemplate(Copy, NVectors, XLength, YLength)

#------------------------------------------------------------------------------------------------

######################################################################
# Validators:
#     In C++ validators are templated meaing the correct type
#     must be called from Python. A Python class of the
#     same name as the C++ type is used so that the user can indicate
#     which validator the require and then its type is deduced from
#     the type of the parameter that has been declared
#
######################################################################
def _createCPPValidator(prop_type, py_valid):
    if isinstance(py_valid, BoundedValidator):
        return _cppBoundedValidator(prop_type, py_valid.lower, py_valid.upper)
    elif isinstance(py_valid, ArrayBoundedValidator):
        return _cppArrayBoundedValidator(prop_type, py_valid.lower, py_valid.upper)
    elif isinstance(py_valid, MandatoryValidator):
        return _cppMandatoryValidator(prop_type)
    elif isinstance(py_valid, ListValidator):
        return _cppListValidator(prop_type, py_valid.options)
    else:
        return None

#------------------- Validator types ---------------------------

# Bounded
class BoundedValidator(object):
    
    def __init__(self, Lower = None, Upper = None):
        if Lower == None and Upper == None:
            raise TypeError("Cannot create BoundedValidator with both Lower and Upper limit unset.")
        self.lower = Lower
        self.upper = Upper

def _cppBoundedValidator(prop_type, low, up):
    if prop_type == int:
        validator = BoundedValidator_int()
    elif prop_type == float:
        validator = BoundedValidator_dbl()
    else:
        raise TypeError("Cannot create BoundedValidator for given property type.")
    
    if low != None:
        validator.setLower(low)
    if up != None:
        validator.setUpper(up)

    return validator

class ArrayBoundedValidator(object):
    def __init__(self, Lower=None, Upper=None):
        if Lower == None and Upper == None:
            raise TypeError("Cannot create ArrayBoundedValidator with both Lower and Upper limit unset")
        self.lower = Lower
        self.upper = Upper

def _cppArrayBoundedValidator(prop_type, low, up):
    if prop_type == int:
        validator = ArrayBoundedValidator_int()
    elif prop_type == float:
        validator = ArrayBoundedValidator_dbl()
    else:
        raise TypeError("Cannot create ArrayBoundedValidator for property type '%s'" % prop_type)

    if low != None:
        validator.setLower(low)
    if up != None:
        validator.setUpper(up)

    return validator
      
# Mandatory
class MandatoryValidator(object):
    pass

def _cppMandatoryValidator(prop_type):
    if prop_type == str:
        return MandatoryValidator_str()
    elif prop_type == int:
        return MandatoryValidator_int()
    elif prop_type == float:
        return MandatoryValidator_dbl()
    else:
        raise TypeError("Cannot create MandatoryValidator for given property type.")

# ListValidator
class ListValidator(object):
    
    def __init__(self, options):
        self.options = cpp_list_str()
        for i in options:
            self.options.append(i)
    
def _cppListValidator(prop_type, options):
    if prop_type == str:
        return ListValidator_str(options)
    else:
        raise TypeError("Cannot create ListValidator for the given property type")

########################################################################################

########################################################################################
# Startup code
########################################################################################

#------------------------------------------------------------------------------
# TableWorkspace Operations
#------------------------------------------------------------------------------
def attach_tableworkspaceiterator():
    """Attaches the iterator code to a table workspace."""
    def __iter_method(self):
        class ITableWorkspaceIter:
            def __init__(self, wksp):
                self.__wksp = wksp
                self.__pos = 0
                self.__max = wksp.getRowCount()
            def next(self):
                if self.__pos + 1 > self.__max:
                    raise StopIteration
                self.__pos += 1
                return self.__wksp.row(self.__pos-1)
        return ITableWorkspaceIter(self)
    setattr(ITableWorkspace, "__iter__", __iter_method)
    
def attach_workspacegroupiterator():
    """Attaches the iterator code to a workspace group."""
    def __iter_method(self):
        class WorkspaceGroupIter:
            def __init__(self, wksp):
                self.__members = wksp.getNames()
                self.__pos = 0
                self.__max = wksp.size()
            def next(self):
                if self.__pos + 1 > self.__max:
                    raise StopIteration
                self.__pos += 1
                return mtd[self.__members[self.__pos-1]]
        return WorkspaceGroupIter(self)
    setattr(WorkspaceGroup, "__iter__", __iter_method)
##############################################################################
attach_tableworkspaceiterator()
attach_workspacegroupiterator()

def FrameworkSingleton():
    try:
        getattr(__main__, '__mantid__')
    except AttributeError:
        setattr(__main__, '__mantid__', MantidPyFramework())
    return getattr(__main__, '__mantid__')

mtd = FrameworkSingleton()
#Aliases
mantid = mtd
Mantid = mtd
Mtd = mtd

# Required directories (config service has changed them to absolute paths)
MantidPyFramework._addToPySearchPath(mtd.settings['requiredpythonscript.directories'])
# Now additional user specified directories
MantidPyFramework._addToPySearchPath(mtd.settings['pythonscripts.directories'])

#---------------------------------------------------------------------------------------
def suppressV1Warnings(filename, suppress=True):
    """
        Enable/disable the warnings about the removal of this API coming from the
        given filename. The match will be based on an exact path match. The simplest
        way to use the function is using __file__: suppressV1Warnings(__file__)
        @param filename A filename
        @param suppress If true the warnings are enabled, otherwise they are not (default=True)
    """
    global SUPPRESS_API_WARN
    if suppress:
        SUPPRESS_API_WARN.add(filename)
    else:
        try:
            SUPPRESS_API_WARN.remove(filename)
        except KeyError, exc:
            pass

def warnOnV1MethodCall(frame=None, identifier=None):
    """
        Issue a Mantid log warning about the removal of v1 of the API when a method has
        been called
        If a valid frame is given then the source & line of that frame are printed.
        Use inspect.currentframe() for the current frame and inspect.currentframe().f_back
        for the caller's frame
    """
    if frame:
        src_file = inspect.getsourcefile(frame)
        if src_file in SUPPRESS_API_WARN:
            return
    else:
        src_file = None
    
    src = ""
    if src_file:
        src = " at line %d in '%s'" % (frame.f_lineno,src_file)
    msg = "Warning: Python API v1 call has been made %s" % (src)
    if identifier is not None:
        msg += "[%s]" % str(identifier)
    msg += "\n"
    print msg

#---------------------------------------------------------------------------------------

# -- Issue a general startup warning about the old API --
_msg = "Notice: Version 1 of the Python API ('from mantidsimple import *') will be removed in the next major release\n"
mtd.sendWarningMessage(_msg)
