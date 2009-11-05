import os 
if os.name == 'nt':
    from MantidPythonAPI import FrameworkManager
else:
    from libMantidPythonAPI import FrameworkManager

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

    def __getattribute__(self, attr):
        """
        Reroute a method call to the the stored object
        """
        if attr.startswith('_'):
            return object.__getattribute__(self, attr)
        else:
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
        return self.__add__(rhs)

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
        return self.__sub__(rhs)

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
        return self.__mul__(rhs)

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
        return self.__div__(rhs)

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
    
    def __init__(self, garbage_collector):
        self._gc = garbage_collector

    def create(self, obj):
        proxy = WorkspaceProxy(obj, self)
        self._gc.register(obj.getName(), proxy)
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

#---------------------------------------------------------------------------------------

class MantidPyFramework(FrameworkManager):
    '''
    The main Mantid Framework object. It mostly forwards its calls to the 
    C++ manager but some workspace related things are captured here first
    '''
    
    def __init__(self):
        # Call base class constructor
        super(MantidPyFramework, self).__init__()
        self._garbage_collector = WorkspaceGarbageCollector()
        self._proxyfactory = WorkspaceProxyFactory(self._garbage_collector)

    def getMatrixWorkspace(self, name):
        '''
        Get a matrix workspace by name. Returns a proxy object
        '''
        wksp = self._getRawMatrixWorkspacePointer(name)
        return self._proxyfactory.create(wksp)

    def getTableWorkspace(self, name):
        '''
        Get a table workspace by name. Returns a proxy object
        '''
        wksp = self._getRawTableWorkspacePointer(name)
        return self._proxyfactory.create(wksp)

    def getMatrixWorkspaceGroup(self, name):
        '''
        Get a list of matrix workspaces
        '''
        wksp_grp = self._getRawWorkspaceGroupPointer(name)
        # Build a list of proxy objects
        names = wksp_grp.getNames()
        return [ self.getMatrixWorkspace(w) for w in names[1:] ]        

    def _workspaceRemoved(self, name):
        '''
        Called when a workspace has been removed from the Mantid ADS
        '''
        self._garbage_collector.kill_object(name);

    def _workspaceReplaced(self, name):
        '''
        Called when a workspace has been removed from the Mantid ADS
        '''
        # 99% of the time people are using matrix workspaces but we still need to check
        try:
            wksp = self._getRawMatrixWorkspacePointer(name)
        except(RuntimeError):
            try:
                wksp = self._getTableWorkspacePointer(name)
            except(RuntimeError):
                return
        
        # If we get here we will have a valid wksp object reference
        self._garbage_collector.replace(name, wksp)

    def _workspaceRemoved(self, name):
        '''
        Called when a workspace has been removed from the Mantid ADS
        '''
        self._garbage_collector.kill_object(name);

    def _workspaceAdded(self, name):
        '''
        Called when a workspace has been added to the Mantid ADS.
        '''
        return
    


#-------------------------------------------------------------------------------------------

    
    
