"""
    List of user commands. 
"""

from reducer import Reducer

class ReductionSingleton:
    """ Singleton reduction class """

    ## storage for the instance reference
    __instance = None

    def __init__(self):
        """ Create singleton instance """
        # Check whether we already have an instance
        if ReductionSingleton.__instance is None:
            # Create and remember instance
            ReductionSingleton.__instance = Reducer()

        # Store instance reference as the only member in the handle
        self.__dict__['_ReductionSingleton__instance'] = ReductionSingleton.__instance
        
    @classmethod
    def clean(cls, reducer_cls=None):
        if reducer_cls==None:
            ReductionSingleton.__instance = Reducer()
        else:
            ReductionSingleton.__instance = reducer_cls()
        
    @classmethod
    def run(cls):
        if ReductionSingleton.__instance is not None:
            ReductionSingleton.__instance._reduce()
        ReductionSingleton.clean(ReductionSingleton.__instance.__class__)
        
    def __getattr__(self, attr):
        """ Delegate access to implementation """
        return getattr(self.__instance, attr)

    def __setattr__(self, attr, value):
        """ Delegate access to implementation """
        return setattr(self.__instance, attr, value)

## List of user commands ######################################################
def Clear(reducer_cls=None):
    """
        Clears the Reducer of changes applied by all previous commands
    """
    ReductionSingleton.clean(reducer_cls)

def DataPath(path):
    ReductionSingleton().set_data_path(path)

def Reduce1D():
    return ReductionSingleton().reduce()
        
def AppendDataFile(datafile, workspace=None):
    """
        Append a data file in the list of files to be processed.
        @param datafile: data file to be processed
        @param workspace: optional workspace name for this data file
            [Default will be the name of the file]
    """
    ReductionSingleton().append_data_file(datafile, workspace)
    
    