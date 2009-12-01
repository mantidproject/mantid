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
        output = ''
        for i in range(0, n_names):
            wksp = self._retrieveWorkspace(names[i])
            output += names[i] + '\t-\t'
            if( isinstance(wksp, Mantid.MatrixWorkspace) ):
                output += 'MatrixWorkspace'
            elif( isinstance(wksp, Mantid.ITableWorkspace) ):
                output += 'TableWorkspace'
            else:
                output += 'WorkspaceGroup'
            output += '\n'
        print output
        

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

    def _workspaceRemoved(self, name):
        '''
        Called when a workspace has been removed from the Mantid ADS
        '''
        self._garbage_collector.kill_object(name)

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
