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

# Check whether MANTIDPATH is defined. If so, append it to the PYTHONPATH.
if os.getenv("MANTIDPATH") is not None:
    sys.path.append(os.getenv("MANTIDPATH"))
else:
    framework_file = os.path.abspath(__file__)
    os.environ["MANTIDPATH"]=os.path.split(framework_file)[0] # use the directory

# --- Import the Mantid API ---
if os.name == 'nt':
    from MantidPythonAPI import *
    from MantidPythonAPI import _binary_op
else:
    # 
    # To enable symbol sharing across extension modules (i.e. loaded dynamic libraries)
    # calls to dlopen by Python must also use the RTLD_GLOBAL flag. If the default
    # dlopen flags are used then the Singleton instance symbols will be multiply 
    # defined across libraries and multiple intances Singleton instances can be created
    #
    saved_dlopenflags = sys.getdlopenflags()
    if platform.system() == "Linux":
        try:
            import DLFCN as dynload
        except:
            # Try older module
            try:
                import dl as dynload
            except:
                # If neither is available then this platform is unsupported
                print "Both the DLFCN and dl modules are unavailable."
                print "Cannot run Mantid from stand-alone Python on this platform."
                sys.exit(1)

        sys.setdlopenflags(dynload.RTLD_NOW | dynload.RTLD_GLOBAL)

    from libMantidPythonAPI import *
    from libMantidPythonAPI import _binary_op
    sys.setdlopenflags(saved_dlopenflags)
# --- End of library load ---


#-------------------------------------------------------------------------------
def makeString(value):
    if isinstance(value, list):
        return str(value).lstrip('[').rstrip(']')
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
                      'INPLACE_OR'])


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
        if name in self._refs:
            self._refs[name]._swap(wksp)

    def kill_object(self, name):
        """
        Signal the proxy to nullify its stored reference
        """
        if name in self._refs:
            self._refs[name]._kill_object()
            # Remove the key as we don't want to keep the reference around
            del self._refs[name]

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
            if isinstance(p, MatrixWorkspaceProperty) or isinstance(p, TableWorkspaceProperty) or \
              isinstance(p, EventWorkspaceProperty) or isinstance(p, WorkspaceProperty):
               self.__wkspnames.append(p.value)
        self.__havelist = True

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
        self._pyalg_loader = PyAlgLoader(self)
        self._pyalg_objs = {}

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
        output_table = '\nWorkspace list:\n'
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

    def registerPyAlgorithm(self, algorithm):
        """
        Register an algorithm into the framework
        """
        # Keep the original object alive
        self._pyalg_objs[algorithm.name()] = algorithm
        super(MantidPyFramework, self).registerPyAlgorithm(algorithm)

    def initialise(self, GUI = False):
        """
        Initialise the framework
        """
        if self.__is_initialized == True:
            return
            
        # Welcome everyone to the world of MANTID
        print '\n' + self.settings.welcomeMessage() + '\n'
        # Run through init steps
        self.createPythonSimpleAPI(GUI)
        self._pyalg_loader.load_modules(refresh=False)
        self.createPythonSimpleAPI(GUI)
        self._importSimpleAPIToMain()     

        self.__is_initialized = True
        
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


    def _importSimpleAPIToMain(self):
        simpleapi = 'mantidsimple'
        if simpleapi in sys.modules:
            mod = sys.modules[simpleapi]
            # At startup the pyc file is created too close to the creation of the module itself and successful reloads require that
            # it be removed first
            pyc_file = mod.__file__
            if pyc_file.endswith('.py'):
                pyc_file += 'c'
            try:
                os.remove(pyc_file)
            except OSError:
                # Don't care if it's not there
                pass
            mod = reload(sys.modules[simpleapi])
        else:
            mod = __import__(simpleapi)
        for name in dir(mod):
            if name.startswith('_'):
                continue
            setattr(__main__, name, getattr(mod, name))
            
    def _refreshPyAlgorithms(self):
        # Reload the modules that have been loaded
        self._pyalg_loader.load_modules(refresh=True)
                
    def _retrieveWorkspace(self, name):
        """
        Use the appropriate function to return the workspace that has that name
        """
        # 99% of the time people are using matrix workspaces but we still need to check
        try:
            return self._getRawMatrixWorkspacePointer(name)
        except(RuntimeError):
            try:
                return self._getRawWorkspaceGroupPointer(name)
            except RuntimeError:
                try:
                    return self._getRawTableWorkspacePointer(name)
                except RuntimeError:
                    return None

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

    def _createAlgProxy(self, ialg):
        return IAlgorithmProxy(ialg, self)

      
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
    
    def __init__(self, framework):
        self.framework = framework


    def load_modules(self, refresh=False):
        """
        Import Python modules containing Python algorithms
        """
        dir_list = mtd.getConfigProperty("pythonalgorithms.directories").split(';')
        if len(dir_list) == 0: 
            return

        # Disable factory updates while everything is (re)imported
        self.framework._observeAlgFactoryUpdates(False,False)
        
        # Check defined Python algorithm directories and load any modules
        changes = False
        for path in dir_list:
            if path == '':
                continue
            if self._importAlgorithms(path, refresh):
                changes = True

        # Now connect the relevant signals to monitor for algorithm factory updates
        self.framework._observeAlgFactoryUpdates(True, (refresh and changes))

#
# ------- Private methods --------------
#
    def _importAlgorithms(self, path, refresh):
        # Make sure the directory doesn't contain a trailing slash
        path = path.rstrip("/").rstrip("\\")
        try:
            files = os.listdir(path)
        except(OSError):
            return False
        # Temporarily insert into path
        sys.path.insert(0, path)
        changes = False
        for modname in files:
            pyext = '.py'
            if not modname.endswith(pyext):
                continue
            original = os.path.join(path, modname)
            modname = modname[:-len(pyext)]
            compiled = os.path.join(path, modname + '.pyc')
            if modname in sys.modules and \
               os.path.exists(compiled) and \
               os.path.getmtime(compiled) >= os.path.getmtime(original):
                continue
            try:               
                if self._containsPyAlgorithm(original):
                    if modname in sys.modules:
                        reload(sys.modules[modname])
                    else:
                        __import__(modname)
                    changes = True
            except(StandardError), exp:
                self.framework.sendLogMessage('Error: Importing module "%s" failed". %s' % (modname,str(exp)))
                continue
            except:
                self.framework.sendLogMessage('Error: Unknown error on Python algorithm module import. "%s" skipped' % modname)
                continue

        # Cleanup system path
        del sys.path[0]
        return changes

    def _containsPyAlgorithm(self, modfilename):
        file = open(modfilename,'r')
        line_count = 0
        alg_found = False
        while line_count < self.__CHECKLINES__:
            line = file.readline()
            # EOF
            if line == '':
                alg_found = False
                break
            if line.rfind('PythonAlgorithm') >= 0:
                alg_found = True
                break
            line_count += 1
        file.close()
        return alg_found
    
#-------------------------------------------------------------------------------------------
##
# PyAlgorithm class
##

class PythonAlgorithm(PyAlgorithmBase):
    """
    Base class for all Mantid Python algorithms
    """

    def __init__(self):
        super(PythonAlgorithm,self).__init__()
        # Dictionary of property names/types
        self._proptypes = {}
    
    def clone(self):
        return copy.deepcopy(self)
    
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
                                     Description = '', Type = MatrixWorkspace):
        if Type == MatrixWorkspace:
            decl_fn = self._declareMatrixWorkspace
        elif Type == TableWorkspace:
            decl_fn = self._declareTableWorkspace
        else:
            raise TypeError('Unrecognized type of workspace specified for property "' + PropertyName + '"')

        if Validator == None:
            decl_fn(PropertyName, WorkspaceName, Description, Direction)
        else:
            decl_fn(PropertyName, WorkspaceName, Validator, Description, Direction)
            
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
        elif value_type == WorkspaceProperty:
            if isinstance(Value, WorkspaceProxy):
                Value = Value._getHeldObject()
            if isinstance(Value, MatrixWorkspace):
                self._setMatrixWorkspaceProperty(Name, Value)
            elif isinstance(Value, ITableWorkspace):
                self._setTableWorkspaceProperty(Name, Value)
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
        
    def executeSubAlg(self, algorithm, *args, **kwargs):
        """
        Execute an algorithm as a sub-algorithm.
        The method will set the input/output workspace objects so that they can be
        passed to sub-algs without having to save them in the ADS.
        
        @param algorithm: mantidsimple algorithm function returning an IAlgorithm object
        @param *args: arguments to pass to the instantiated algorithm
        @param **kwargs: keyword arguments to pass to the instantiated algorithm
        @return: algorithm proxy for the executed algorithm
        """
        if not isinstance(algorithm, types.FunctionType):
            raise RuntimeError, "PythonAlgorithm.executeSubAlg expects a function."
        
        algm = self._createSubAlgorithm(algorithm.func_name)
        proxy = mtd._createAlgProxy(algm)
        if not isinstance(proxy, IAlgorithmProxy):
            raise RuntimeError, "PythonAlgorithm.executeSubAlg expects a function returning an IAlgorithm object"                    
        
        # The inspect module has changed in python 2.6 
        if sys.version_info[0]==2 and sys.version_info[1]<6:
            _args = inspect.getargspec(algorithm)[0] 
        else:
            _args = inspect.getargspec(algorithm).args     
        # Go through provided arguments
        for i in range(len(args)):
            if isinstance(args[i], WorkspaceProxy):
                algm._setWorkspaceProperty(_args[i], args[i]._getHeldObject())                
            else:
                algm.setPropertyValue(_args[i], makeString(args[i]).lstrip('? '))
        # Go through keyword arguments
        for key in kwargs:
            if key not in proxy.keys():
                continue           
            if isinstance(kwargs[key], WorkspaceProxy):
                algm._setWorkspaceProperty(key, kwargs[key]._getHeldObject())
            else:             
                algm.setPropertyValue(key, makeString(kwargs[key]).lstrip('? '))
        
        # Execute synchronously        
        algm.setRethrows(True)
        algm.execute()
                
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

# Update search path on posix machines where mantidsimple.py goes to the user directory
if os.name == 'posix':
    outputdir = os.path.expanduser('~/.mantid')
    if not outputdir in sys.path:
        sys.path.append(outputdir)

# Required directories (config service has changed them to absolute paths)
MantidPyFramework._addToPySearchPath(mtd.settings['requiredpythonscript.directories'])
# Now additional user specified directories
MantidPyFramework._addToPySearchPath(mtd.settings['pythonscripts.directories'])
# Finally old scripts key; For backwards compatability add all directories that are one-level below this
_script_dirs = mtd.settings['pythonscripts.directory']
MantidPyFramework._addToPySearchPath(_script_dirs)
if _script_dirs == '':
    _files = []
else:
    try:
        _files = os.listdir(_script_dirs)
    except:
        _files = []

for _file in _files:
    _fullpath = os.path.join(_script_dirs,_file)
    if not 'svn' in _file and os.path.isdir(_fullpath):
        MantidPyFramework._addToPySearchPath(_fullpath)
