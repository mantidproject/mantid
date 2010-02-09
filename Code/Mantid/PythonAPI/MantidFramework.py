import os, sys, types, copy, __builtin__,__main__
if os.name == 'nt':
    from MantidPythonAPI import *
else:
    from libMantidPythonAPI import *

"""
Top-level interface classes for Mantid.
"""

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
class WorkspaceProxy(object):
    '''
    A proxy object that stores a workspace instance. When the workspace is deleted
    from the ADS in Mantid, the object reference held here is set to "None"
    '''

    def __init__(self, obj, factory):
        """
        Create a proxy for the given object
        """
        self.__obj = obj
        self.__factory = factory

    def isGroup(self):
        '''
        Is the data object a WorkspaceGroup or not
        '''
        if isinstance(self.__obj, WorkspaceGroup):
            return True
        else:
            return False

    def __getitem__(self, index):
        if self.isGroup():
            return self.__factory.create(self.__obj.getNames()[index])
        else:
            return self
        

    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        return getattr(self.__obj, attr)

    def _getHeldObject(self):
        """
        Return the underlying object
        """
        return self.__obj

    def _kill_object(self):
        '''
        Release the stored instance
        '''
        self.__obj = None

    def _swap(self, obj):
        '''
        Swap an object so that the proxy now refers to this object
        '''
        self.__obj = obj

    def __str__(self):
        '''
        Return a string representation of the proxied object
        '''
        return str(self.__obj)

    def __repr__(self):
        '''
        Return a string representation of the proxied object
        '''
        return `self.__obj`

    def __add__(self, rhs):
        '''
        Sum the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self.__obj + rhs.__obj)
        else:
            return self.__factory.create(self.__obj + rhs)

    def __radd__(self, rhs):
        '''
        Sum the proxied objects and return a new proxy managing that object
        '''
        return self.__factory.create(rhs + self.__obj)

    def __iadd__(self, rhs):
        '''
        In-place ssum the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            self.__obj += rhs.__obj
        else:
            self.__obj += rhs
        return self

    def __sub__(self, rhs):
        '''
        Subtract the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self.__obj - rhs.__obj)
        else:
            return self.__factory.create(self.__obj - rhs)

    def __rsub__(self, rhs):
        '''
        Subtract the proxied objects and return a new proxy managing that object
        '''
        return self.__factory.create(rhs - self.__obj)

    def __isub__(self, rhs):
        '''
        In-place subtract the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            self.__obj -= rhs.__obj
        else:
            self.__obj -= rhs
        return self

    def __mul__(self, rhs):
        '''
        Multiply the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self.__obj * rhs.__obj)
        else:
            return self.__factory.create(self.__obj * rhs)

    def __rmul__(self, rhs):
        '''
        Multiply the proxied objects and return a new proxy managing that object
        '''
        return self.__factory.create(rhs * self.__obj)

    def __imul__(self, rhs):
        '''
        In-place multiply the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            self.__obj *= rhs.__obj
        else:
            self.__obj *= rhs
        return self

    def __div__(self, rhs):
        '''
        Divide the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self.__obj / rhs.__obj)
        else:
            return self.__factory.create(self.__obj / rhs)

    def __rdiv__(self, rhs):
        '''
        Divide the proxied objects and return a new proxy managing that object
        '''
        return self.__factory.create(rhs / self.__obj)

    def __idiv__(self, rhs):
        '''
        In-place divide the proxied objects and return a new proxy managing that object
        '''
        if isinstance(rhs, WorkspaceProxy) :
            self.__obj /= rhs.__obj
        else:
            self.__obj /= rhs
        return self

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
    '''
    A register of workspaces that have been retrieved from Mantid
    '''
    
    def __init__(self):
        self._refs = {}

    def register(self, name, proxy):
        '''
        Register a name and reference to the store
        '''
        self._refs[name] = proxy

    def replace(self, name, wksp):
        '''
        Replace an object reference within a proxy
        '''
        if name in self._refs:
            self._refs[name]._swap(wksp)

    def kill_object(self, name):
        '''
        Signal the proxy to nullify its stored reference
        '''
        if name in self._refs:
            self._refs[name]._kill_object()
            # Remove the key as we don't want to keep the reference around
            del self._refs[name]

    def kill_all(self):
        '''
        Kill all references to data objects
        '''
        # Note here that a simple clear won't do as it won't set the object reference to None 
        for w in self._refs:
            self._refs[w]._kill_object()
        self._refs.clear()
#---------------------------------------------------------------------------------------

class IAlgorithmProxy(object):
    '''
    A proxy object for IAlgorithm returns
    '''
    
    def __init__(self, ialg, framework):
        self.__alg = ialg
        self.__framework = framework
        self.__havelist = False
        self.__wkspnames = []
        
    def workspace(self):
        return self._retrieveWorkspaceByIndex(0)
        
    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        return getattr(self.__alg, attr)

    def __getitem__(self, index):
        return self._retrieveWorkspaceByIndex(index)

    def _retrieveWorkspaceByIndex(self, index):
        if self.__havelist == False:
            self._createWorkspaceList()

        if len(self.__wkspnames[index]) > 0:
            return self.__framework[self.__wkspnames[index]]
        else:
            return None

    def _createWorkspaceList(self):
        # Create a list for the output workspaces
        props = self.__alg.getProperties()
  
        for p in props:
            if p.direction() != 1:
                continue
            if isinstance(p, MatrixWorkspaceProperty) or \
                    isinstance(p, TableWorkspaceProperty) or \
                    isinstance(p, WorkspaceProperty):
                self.__wkspnames.append(p.value())
        self.__havelist = True

#---------------------------------------------------------------------------------------

class MantidPyFramework(FrameworkManager):
    '''
    The main Mantid Framework object. It mostly forwards its calls to the 
    C++ manager but some workspace related things are captured here first
    '''
    def __init__(self, gui_exts = False):
        # Call base class constructor
        super(MantidPyFramework, self).__init__()
        self._garbage_collector = WorkspaceGarbageCollector()
        self._proxyfactory = WorkspaceProxyFactory(self._garbage_collector, self)
        self._algs = {}
        self._reloader = None
 
    def list(self):
        '''
        Print a list of the workspaces stored in Mantid along with their type
        '''
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
            output_table += '\t' + row[0].ljust(max_width, ' ') + '-   ' + row[1]

        print output_table

    def __getitem__(self, key):
        '''
        Enables the framework to be used in a dictionary like manner.
        It returns the MatrixWorkspace proxy for the given name
        '''
        return self._proxyfactory.create(self._retrieveWorkspace(key))

    def registerPyAlgorithm(self, algorithm):
        '''
        Register an algorithm into the framework
        '''
        # This keeps the original object alive
        self._algs[algorithm.name()] = algorithm
        super(MantidPyFramework, self).registerPyAlgorithm(algorithm)

##                     ##
## "Private functions" ##
##                     ##
    def _initPythonAlgorithms(self, reload = False):
        # Disable factory updates while everything is reimported
        self._observeAlgFactoryUpdates(False,False)
        
        if self._reloader == None:
            self._reloader = RollbackImporter()

        # Check defined Python algorithm directories and load any modules
        dir_list = self.getConfigProperty('pythonalgorithms.directories').split(';')
        if len(dir_list) > 0:
            changes = False
            for d in dir_list:
                if d != '':
                    if self._importPyAlgorithms(d, reload) == True:
                        changes = True

        # Now connect the relevant signals to monitor for algorithm factory updates
        self._observeAlgFactoryUpdates(True, reload and changes)

    def _importPyAlgorithms(self, dir, reload):
        try:
            files = os.listdir(dir)
        except(OSError):
            return
        # Temporarily insert into path
        sys.path.insert(0, dir)
        changes = False
        for modname in files:
            if not modname.endswith('.py'):
                continue
            original = os.path.join(dir, modname)
            modname = modname.rstrip('.py')
            compiled = os.path.join(dir, modname + '.pyc')
            if modname in sys.modules and \
               os.path.exists(compiled) and \
               os.path.getmtime(compiled) >= os.path.getmtime(original):
                continue
            try:
                __import__(modname)
            except ImportError, details:
                print details
            changes = True

        # Cleanup system path
        del sys.path[0]
        return changes

    def _importSimpleAPIToGlobal(self):
        simpleapi = 'mantidsimple'
        if simpleapi in sys.modules:
            mod = reload(sys.modules[simpleapi])
        else:
            mod = __import__(simpleapi)
        for name in dir(mod):
            if name == '__name__':
                continue
            setattr(__main__, name, getattr(mod, name))
            
    def _refreshPyAlgorithms(self):
        # Pass to the loader proxy
        if self._reloader != None:
            self._reloader.uninstall()        
            # Now reload the modules that were loaded at startup
            self._initPythonAlgorithms(reload = True)
        
    def _retrieveWorkspace(self, name):
        '''
        Use the appropriate function to return the workspace that has that name
        '''
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
        '''
        Called when a workspace has been removed from the Mantid ADS
        '''
        self._garbage_collector.kill_object(name);

    def _workspaceReplaced(self, name):
        '''
        Called when a workspace has been removed from the Mantid ADS
        '''
        wksp = self._retrieveWorkspace(name)
        if wksp != None:
            self._garbage_collector.replace(name, wksp)

    def _workspaceStoreCleared(self):
        '''
        Called when the ADS is cleared
        '''
        self._garbage_collector.kill_all()

    def _algorithmFactoryUpdated(self):
        '''
        Called in reponse to an algorithm factory update
        '''
        # Reload the simple api
        self._importSimpleAPIToGlobal()

    def _createAlgProxy(self, ialg):
        return IAlgorithmProxy(ialg, self)

      
# *** Legacy functions ***

    def getMatrixWorkspace(self, name):
        '''
        Get a matrix workspace by name. Returns a proxy object
        '''
        return self._proxyfactory.create(self._getRawMatrixWorkspacePointer(name))
    
    def getTableWorkspace(self, name):
        '''
        Get a table workspace by name. Returns a proxy object
        '''
        return self._proxyfactory.create(self._getRawTableWorkspacePointer(name))
        
    def getMatrixWorkspaceGroup(self, name):
        '''
        Get a list of matrix workspaces
        '''
        wksp_grp = self._getRawWorkspaceGroupPointer(name)
        # Build a list of proxy objects
        names = wksp_grp.getNames()
        return [ self.getMatrixWorkspace(w) for w in names[1:] ]

#------------------------------------------------------------------------------------------

##
# Inspired by PyUnit, a class that handles reloading modules cleanly 
# See http://pyunit.sourceforge.net/notes/reloading.html
##
class RollbackImporter:
  
    def __init__(self):
        "Creates an instance and installs as the global importer"
        self.realImport = __builtin__.__import__
        __builtin__.__import__ = self._import
        self.pyalg_modules = {}
        
    def _import(self, name, globals=None, locals=None, fromlist=[]):
        result = apply(self.realImport, (name, globals, locals, fromlist))
        if self._containsPyAlgorithm(result):
            self.pyalg_modules[name] = self._findPathToModule(name)
        return result

    def _findPathToModule(self, name):
        for p in sys.path:
            filename = os.path.join(p,name + '.py')
            if os.path.exists(filename):
                return p
            
        return ''

    def _containsPyAlgorithm(self, module):
        # Check attributes and check if there are any Python algorithms
        attrs = dir(module)
        for attname in attrs:
            att = getattr(module, attname)
            if type(att) == type(PythonAlgorithm) and issubclass(att, PythonAlgorithm) and att.__name__ != 'PythonAlgorithm':
                return True
        return False
        
    def uninstall(self):
        for modname in self.pyalg_modules.keys():
            if modname in sys.modules:
                mpath = self.pyalg_modules[modname]
                compiled = os.path.join(mpath, modname + '.pyc')
                original = os.path.join(mpath, modname + '.py')
                if os.path.getmtime(original) > os.path.getmtime(compiled):
                    del( sys.modules[modname] )

#-------------------------------------------------------------------------------------------
##
# PyAlgorithm class
##

class PythonAlgorithm(PyAlgorithmBase):
    '''
    Base class for all Mantid Python algorithms
    '''
    # Dictionary of property names/types
    _proptypes = {}
    
    def __init__(self):
        super(PythonAlgorithm,self).__init__()

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
        '''
        Declare a property for this algorithm
        '''
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
        '''
        Retrieve a property value for the given name
        '''
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

def FrameworkSingleton():
    try:
        getattr(__main__, '__mantid__')
    except AttributeError:
        setattr(__main__, '__mantid__', MantidPyFramework())
    return getattr(__main__, '__mantid__')

if os.name == 'posix':
    sys.path.append(os.path.expanduser('~/.mantid'))

mtd = FrameworkSingleton()
mantid = mtd
Mantid = mtd
Mtd = mtd
