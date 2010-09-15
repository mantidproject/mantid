"""Access the Mantid Framework.
"""
import os
import platform
import sys
import types
import copy
import sets
import __builtin__
import __main__


# --- Import the Mantid API ---
if os.name == 'nt':
    from MantidPythonAPI import *
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
class ProxyObject(object):
    """Base class for all objects acting as proxys
    """
    def __init__(self, toproxy):
        self.__obj = toproxy

    def _getHeldObject(self):
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


class WorkspaceProxy(ProxyObject):
    """
    A proxy object that stores a workspace instance. When the workspace is deleted
    from the ADS in Mantid, the object reference held here is set to "None"
    """

    def __init__(self, obj, factory):
        """
        Create a proxy for the given object
        """
        super(WorkspaceProxy, self).__init__(obj)
        self.__factory = factory

    def isGroup(self):
        """
        Is the data object a WorkspaceGroup or not
        """
        if isinstance(self._getHeldObject(), WorkspaceGroup):
            return True
        else:
            return False

    def __getitem__(self, index):
        """
        If we are a group then return a member, else return self
        """
        if self.isGroup():
            return self.__factory.create(self._getHeldObject().getNames()[index])
        else:
            raise AttributeError('Index invalid, object is not a group.')

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

    def __add__(self, rhs):
        """
        Sum the proxied objects and return a new proxy managing that object
        """
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self._getHeldObject() + rhs._getHeldObject())
        else:
            return self.__factory.create(self._getHeldObject() + rhs)

    def __radd__(self, rhs):
        """
        Sum the proxied objects and return a new proxy managing that object
        """
        return self.__factory.create(rhs + self._getHeldObject())

    def __iadd__(self, rhs):
        """
        In-place ssum the proxied objects and return a new proxy managing that object
        """
        result = self._getHeldObject()
        if isinstance(rhs, WorkspaceProxy):
            rhs = rhs._getHeldObject()
        result += rhs
        return self

    def __sub__(self, rhs):
        """
        Subtract the proxied objects and return a new proxy managing that object
        """
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self._getHeldObject() - rhs._getHeldObject())
        else:
            return self.__factory.create(self._getHeldObject() - rhs)

    def __rsub__(self, rhs):
        """
        Subtract the proxied objects and return a new proxy managing that object
        """
        return self.__factory.create(rhs - self._getHeldObject())

    def __isub__(self, rhs):
        """
        In-place subtract the proxied objects and return a new proxy managing that object
        """
        result = self._getHeldObject()
        if isinstance(rhs, WorkspaceProxy):
            rhs = rhs._getHeldObject()
        result -= rhs
        return self

    def __mul__(self, rhs):
        """
        Multiply the proxied objects and return a new proxy managing that object
        """
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self._getHeldObject() * rhs._getHeldObject())
        else:
            return self.__factory.create(self._getHeldObject() * rhs)

    def __rmul__(self, rhs):
        """
        Multiply the proxied objects and return a new proxy managing that object
        """
        return self.__factory.create(rhs * self._getHeldObject())

    def __imul__(self, rhs):
        """
        In-place multiply the proxied objects and return a new proxy managing that object
        """
        result = self._getHeldObject()
        if isinstance(rhs, WorkspaceProxy):
            rhs = rhs._getHeldObject()
        result *= rhs
        return self

    def __div__(self, rhs):
        """
        Divide the proxied objects and return a new proxy managing that object
        """
        if isinstance(rhs, WorkspaceProxy) :
            return self.__factory.create(self._getHeldObject() / rhs._getHeldObject())
        else:
            return self.__factory.create(self._getHeldObject() / rhs)

    def __rdiv__(self, rhs):
        """
        Divide the proxied objects and return a new proxy managing that object
        """
        return self.__factory.create(rhs / self._getHeldObject())

    def __idiv__(self, rhs):
        """
        In-place divide the proxied objects and return a new proxy managing that object
        """
        result = self._getHeldObject()
        if isinstance(rhs, WorkspaceProxy):
            rhs = rhs._getHeldObject()
        result /= rhs
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
        
    def workspace(self):
        return self._retrieveWorkspaceByIndex(0)
        
    def __getattr__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        return getattr(self._getHeldObject(), attr)

    def __getitem__(self, index):
        return self._retrieveWorkspaceByIndex(index)

    def _retrieveWorkspaceByIndex(self, index):
        if self.__havelist == False:
            self._createWorkspaceList()

        if len(self.__wkspnames) > 0:
            return self.__framework[self.__wkspnames[index]]
        else:
            raise RuntimeError("'%s' has no output workspaces." % self._getHeldObject().name())

    def _createWorkspaceList(self):
        # Create a list for the output workspaces
        props = self._getHeldObject().getProperties()
        for p in props:
            if p.direction() != 1:
                continue
            if isinstance(p, MatrixWorkspaceProperty) or isinstance(p, TableWorkspaceProperty) or \
              isinstance(p, EventWorkspaceProperty) or isinstance(p, WorkspaceProperty):
               self.__wkspnames.append(p.value())
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
            output_table += '\t' + row[0].ljust(max_width, ' ') + '-   ' + row[1]

        print output_table

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
            os.remove(pyc_file)
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
