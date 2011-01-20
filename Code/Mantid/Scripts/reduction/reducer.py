"""
    Base reduction class. Hold a list of data and a list of reduction steps to apply to them.
    
    Pseudo code example:
    
    r = Reducer()
    r.set_instrument( SANSInstrument() )
    r.append_step( ReductionStep() )
    r.reduce()
    
    The ReductionStep object is initialized before being passed to the Reducer.
    The Reducer will call ReductionStep.execute() when Reducer.reduce() is called.
    
    The ReductionStep.execute() method takes two arguments, a reference to the
    Reducer itself and the name of the workspace to apply the step to (usually a data set). 
    The Reducer object reference is passed so that the reduction step can access
    instrument settings.
    
"""
import os
import time
import types
from instrument import Instrument
import MantidFramework
import mantidsimple


## Version number
__version__ = '1.0'
      
def validate_step(f):
    """
        Decorator for Reducer methods that need a ReductionStep
        object as its first argument.
        
        Example:
            @validate_step
            def some_func(self, reduction_step):
                [...]
    """
    
    def validated_f(reducer, algorithm, *args, **kwargs):
        """
            Wrapper function around the function f.
            The function ensures that the algorithm parameter
            is a sub-class of ReductionStep
        """
            
        if issubclass(algorithm.__class__, ReductionStep) or algorithm is None:
            # If we have a ReductionStep object, just use it.
            # "None" is allowed as an algorithm (usually tells the reducer to skip a step)
            return f(reducer, algorithm)
        
        if isinstance(algorithm, types.StringType):
            # If we have a string, see if there exists a mantidsimple
            # function that can be used
            algorithm = eval(algorithm, mantidsimple.__dict__, mantidsimple.__dict__)
            
        if isinstance(algorithm, types.FunctionType):
            # If the algorithm is a function, in which case
            # we expect it to produce an algorithm proxy
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algm = algorithm
                def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
                    proxy = self.algm(*args, execute=False, **kwargs)
                    if not isinstance(proxy, MantidFramework.IAlgorithmProxy):
                        raise RuntimeError, "Reducer expects a ReductionStep or a function returning an IAlgorithmProxy object"                    
                    _algm = proxy._getHeldObject()
                    if outputworkspace is None:
                        outputworkspace = inputworkspace 
                    _argspec = proxy.keys()                       
                    if "InputWorkspace" in _argspec:
                        print "setting input"
                        _algm.setPropertyValue("InputWorkspace", inputworkspace)
                    if "OutputWorkspace" in _argspec:
                        _algm.setPropertyValue("OutputWorkspace", outputworkspace)
                    mantidsimple.execute_algorithm(proxy)
                    
            return f(reducer, _AlgorithmStep())
        else:
            raise RuntimeError, "%s expects a ReductionStep object, found %s" % (f.__name__, algorithm.__class__)
    return validated_f
    
class Reducer(object):
    """
        Base reducer class. Instrument-specific reduction processes should be
        implemented in a child of this class.
    """
    
    ## Instrument configuration object
    instrument = None    
    ## Path for data files
    _data_path = '.'
    ## List of data files to process
    _data_files = {}
    ## List of workspaces that were modified
    _dirty = []
    ## List of reduction steps
    _reduction_steps = []
    ## Log
    log_text = ''
        
    def __init__(self):
        self._data_files = {}
        self._reduction_steps = []
        
    def set_instrument(self, configuration):
        if issubclass(configuration.__class__, Instrument):
            self.instrument = configuration
        else:
            raise RuntimeError, "Reducer.set_instrument expects an %s object, found %s" % (Instrument, configuration.__class__)
        
    def dirty(self, workspace):
        """
            Flag a workspace as dirty when the data has been modified
        """
        if workspace not in self._dirty:
            self._dirty.append(workspace)
        
    def clean(self, workspace):
        """
            Remove the dirty flag on a workspace
        """
        if workspace in self._dirty:
            self._dirty.remove(workspace)
            
    def is_clean(self, workspace):
        """
            Returns True if the workspace is clean
        """
        if workspace in self._dirty:
            return False
        return True
    
    def set_data_path(self, path):
        """
            Set the path for data files
            @param path: data file path
        """
        if os.path.isdir(path):
            self._data_path = path
        else:
            raise RuntimeError, "Reducer.set_data_path: provided path is not a directory (%s)" % path
        
    def _full_file_path(self, filename):
        """
            Prepends the data folder path and returns a full path to the given file.
            Raises an exception if the file doesn't exist.
            @param filename: name of the file to create the full path for
        """
        filepath = os.path.join(self._data_path, filename)

        # Check that the file exists
        if os.path.isfile(filepath):
            return filepath
        elif os.path.isfile(filename):
            # If not, check in the working directory
            return filename

        system_path = MantidFramework.FileFinder.getFullPath(filename).strip()
        
        if system_path :
            return system_path
        else:
            # If not, raise an exception
            raise RuntimeError, 'Could not load file "%s"' % filename
        
    @validate_step
    def append_step(self, reduction_step):
        """
            Append a reduction step
            @param reduction_step: ReductionStep object
        """
        if reduction_step is None:
            return None

        self._reduction_steps.append(reduction_step)

        return reduction_step
        
    def append_data_file(self, data_file, workspace=None):
        """
            Append a file to be processed.
            @param data_file: name of the file to be processed
            @param workspace: optional name of the workspace for this data,
                default will be the name of the file 
        """
        # Check that the file exists
        self._full_file_path(data_file)
        
        if workspace is None:
            workspace = extract_workspace_name(data_file)
            
        self._data_files[workspace] = data_file 
    
    def pre_process(self):
        """
            Reduction steps that are meant to be executed only once per set
            of data files. After this is executed, all files will go through
            the list of reduction steps.
        """
        pass
    
    def reduce(self):
        """
            Go through the list of reduction steps
        """
        # Log text
        self.log_text = "%s reduction - %s\n" % (self.instrument.name(), time.ctime())
        # Check that an instrument was specified
        if self.instrument is None:
            raise RuntimeError, "Reducer: trying to run a reduction with an instrument specified"

        # Go through the list of steps that are common to all data files
        self.pre_process()

        # Go through the list of files to be reduced
        for file_ws in self._data_files:
            for item in self._reduction_steps:
                result = item.execute(self, file_ws)
                if result is not None and len(str(result))>0:
                    self.log_text += "%s\n" % str(result)        

        #any clean up, possibly removing workspaces 
        self.post_process()
    
        return self.log_text
    
    
class ReductionStep(object):
    """
        Base class for reduction steps
    """ 
    def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
        """
            Implemented the reduction step.
            @param reducer: Reducer object for which the step is executed
            @param inputworkspace: Name of the workspace to apply this step to
            @param outputworkspace: Name of the workspace to have as an output. If this is None it will be set to inputworkspace
        """
        raise NotImplemented

class AlgoReductionStep(ReductionStep):
    """
       Class that wraps an algorithm as a reducer
    """
    def __init__(self, algo):
        super(AlgoReductionStep, self).__init__()
        self._algo = algo
        self.__init_args()

    def __repr__(self):
        return "AlgoReductionStep(%s)" % self._algo.__name__

    def __init_args(self):
        # determine what variables exist in the underlying algorithm
        import inspect
        self._argspec = inspect.getargspec(self._algo)

        # set all of the values to none and make them accessible
        args = self._argspec.args
        for arg in args:
            self.__dict__[arg] = None

        # set the default values that the underlying algorithm uses
        defaults = self._argspec.defaults
        indices = [-1*(i+1) for i in range(len(defaults))]
        for i in indices:
            self.__dict__[args[i]] = defaults[i]

    def execute(self, reducer=None, inputworkspace=None, outputworkspace=None):
        """
           Execute the wrapped algorithm
        """
        if outputworkspace is None:
            outputworkspace = inputworkspace 

        # construct the argument list
        args = []
        for arg in self._argspec.args:
            if arg == "InputWorkspace" and inputworkspace is not None:
                args.append(inputworkspace)
            elif arg == "OutputWorkspace" and outputworkspace is not None:
                args.append(outputworkspace)
            else:
                args.append(self.__dict__[arg])

        # call the algorithm
        self._algo(*args)


def extract_workspace_name(filepath, suffix=''):
    """
        Returns a default workspace name for a given data file path.
        
        @param filepath: path of the file to generate a workspace name for
        @param suffix: string to append to name
    """
    (head, tail) = os.path.split(filepath)
    basename, extension = os.path.splitext(tail)
    
    #TODO: check whether the workspace name is already in use
    #      and modify it if it is. 

    return basename+suffix
    
