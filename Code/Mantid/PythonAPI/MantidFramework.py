import os
import types
if os.name == 'nt':
    import MantidPythonAPI as Mantid
else:
    import libMantidPythonAPI as Mantid

"""
Top-level interface classes for Mantid.
"""

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
        if isinstance(self.__obj, Mantid.WorkspaceGroup):
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
            if isinstance(p, Mantid.MatrixWorkspaceProperty) or \
                    isinstance(p, Mantid.TableWorkspaceProperty) or \
                    isinstance(p, Mantid.WorkspaceProperty):
                self.__wkspnames.append(p.value())
        self.__havelist = True

#---------------------------------------------------------------------------------------

class MantidPyFramework(Mantid.FrameworkManager):
    '''
    The main Mantid Framework object. It mostly forwards its calls to the 
    C++ manager but some workspace related things are captured here first
    '''
    
    def __init__(self):
        # Call base class constructor
        super(MantidPyFramework, self).__init__()
        self._garbage_collector = WorkspaceGarbageCollector()
        self._proxyfactory = WorkspaceProxyFactory(self._garbage_collector, self)

    # Enables mtd['name'] syntax
    def __getitem__(self, key):
        '''
        Enables the framework to be used in a dictionary like manner.
        It returns the MatrixWorkspace proxy for the given name
        '''
        return self._proxyfactory.create(self._retrieveWorkspace(key))

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
            if( isinstance(wksp, Mantid.MatrixWorkspace) ):
                output[i][1] = 'MatrixWorkspace'
            elif( isinstance(wksp, Mantid.ITableWorkspace) ):
                output[i][1] = 'TableWorkspace'
            else:
                output[i][1] = 'WorkspaceGroup'
        
        max_width += 3
        output_table = '\nWorkspace list:\n'
        for row in output:
            output_table += '\t' + row[0].ljust(max_width, ' ') + '-   ' + row[1]

        print output_table
        
# *** "Private" functions
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


#-------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------
##
# PyAlgorithm class
##
class PythonAlgorithm(Mantid.PyAlgorithm):
    '''
    Base class for all Mantid Python algorithms
    '''
    # Dictionary of property names/types
    _proptypes = {}
        
    def name(self):
        raise NotImplementedError('name() method must be defined for a Python algorithm')

    def version(self):
        return 1
        
    def category(self):
        return 'PythonAlgorithm'

    def PyInit(self):
        raise NotImplementedError('PyInit() method must be defined for a Python algorithm')

    def PyExec(self):
        raise NotImplementedError('PyExec() method must be defined for a Python algorithm')

    # declareProperty "wrapper" function so that we don't need separate named functions for each type
    def declareProperty(self, Name, DefaultValue, Direction = Mantid.Direction.Input, Description = '', ):
        '''
        Declare a property for this algorithm
        '''
        # Test default value type and call the relevant function
        if isinstance(DefaultValue, int) or isinstance(DefaultValue, long):
            decl_func = self.declareProperty_int
            type = int
        elif isinstance(DefaultValue, float):
            decl_func = self.declareProperty_dbl
            type = float
        elif isinstance(DefaultValue, bool) or isinstance(DefaultValue, str):
            decl_func = self.declareProperty_str
            type = str
            if isinstance(DefaultValue, bool):
                DefaultValue = str(int(DefaultValue))
                type = bool
            else:
                pass
        else:
            raise TypeError('Unrecognized type for property "' + Name + '"')
        
        decl_func(Name, DefaultValue, Description, Direction)
        self._mapPropertyToType(Name, type)

    # Specialized version for workspaces
    def declareWorkspaceProperty(self, PropertyName, WorkspaceName, Direction, Description = '', \
                                     Type = Mantid.MatrixWorkspace):
        if Type == Mantid.MatrixWorkspace:
            self._declareMatrixWorkspace(PropertyName, WorkspaceName, Description, Direction)
        elif Type == Mantid.Tableworkspace:
            self._declareTableWorkspace(PropertyName, WorkspaceName, Description, Direction)
        else:
            raise TypeError('Unrecognized type of workspace specified for property "' + PropertyName + '"')
        
        self._mapPropertyToType(PropertyName, Mantid.WorkspaceProperty)

    # Specialized version for FileProperty
    def declareFileProperty(self, Name, DefaultValue, Type, Exts = [], Direction = Mantid.Direction.Input,\
                                Description = ''):
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

        if prop_type == type(1):
            return self.getProperty_int(Name)
        elif prop_type == type(1.0):
            return self.getProperty_dbl(Name)
        elif prop_type == type(bool):
            return bool(self.getPropertyValue(Name))
        elif prop_type == type(''):
            return self.getPropertyValue(Name)
        elif issubclass(prop_type, Mantid.WorkspaceProperty):
            return self.getPropertyValue(Name)
        else:
            raise TypeError('Unrecognized type for property "' + Name + '"')

    # Wrapper around setPropertyValue
    def setProperty(self, Name, Value):
        try:
            prop_type = self._proptypes[Name]
        except KeyError:
            raise KeyError('Attempting to set unknown property "' + Name + '"')
        
        if isinstance(Value, str):
            self.setPropertyValue(Name, Value)
        elif isinstance(Value, bool):
            self.setPropertyValue(Name, str(int(Value)))
        elif issubclass(prop_type, Mantid.WorkspaceProperty):
            if isinstance(Value, Mantid.MatrixWorkspace):
                self._setMatrixWorkspaceProperty(Name, Value)
            elif isinstance(Value, Mantid.TableWorkspace):
                self._setTableWorkspaceProperty(Name, Value)
            else:
                raise TypeError('Attempting to set workspace property with value that is not a workspace.')
        else:
            self.setPropertyValue(Name, str(Value))
            
    # Execute a string that declares a property and keep track of the registered type
    def _mapPropertyToType(self, name, prop_type):
        self._proptypes[name] = prop_type

#------------------------------------------------------------------------------------------------

###
 # Factory Function
###
def BoundedValidator(lower = None, upper = None):
    if isinstance(lower, None) and isinstance(upper,None):
        raise TypeError("Cannot create BoundedValidator with both lower and upper limit unset.")
    
    if isinstance(lower, None):
        if isinstance(upper, int):
            b = IntBoundedValidator()
        elif isinstance(upper, float):
            b = DblBoundedValidator()
        else:
            raise TypeError("Invalid type for upper bound BoundedValidator")
        b.setUpper(upper)
    elif isinstance(upper, None):
        if isinstance(lower, int):
            b = IntBoundedValidator()
        elif isinstance(lower, float):
            b = DblBoundedValidator()
        else:
            raise TypeError("Invalid type for lower bound of BoundedValidator")
        b.setLower(lower)
    else:
        if isinstance(lower, int):
            if isinstance(upper, int):
                return BoundedValidator_int(lower,upper)
            elif isinstance(upper, float):
                return BoundedValidator_dbl(float(lower),upper)
            else:
                raise TypeError("Invalid type for upper value of BoundedValidator")
        elif isinstance(lower, float):
            if isinstance(upper, float):
                return BoundedValidator_dbl(lower,upper)
            elif isinstance(upper, int):
                return BoundedValidator_dbl(lower,float(upper))
            else:
                raise TypeError("Invalid type for upper value of BoundedValidator")
        else:
            raise TypeError("Invalid type for lower value of BoundedValidator")
        
