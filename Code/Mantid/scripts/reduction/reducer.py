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
import sys
import time
import types
import inspect
from instrument import Instrument
import MantidFramework
import mantidsimple


## Version number
__version__ = '1.0'
      
def validate_loader(f):

    def validated_f(reducer, algorithm, *args, **kwargs):
        if issubclass(algorithm.__class__, ReductionStep) or algorithm is None:
            # If we have a ReductionStep object, just use it.
            # "None" is allowed as an algorithm (usually tells the reducer to skip a step)
            return f(reducer, algorithm)
        
        if isinstance(algorithm, types.FunctionType):
            # If we get a function, assume its name is an algorithm name
            algorithm = algorithm.func_name

        if isinstance(algorithm, MantidFramework.IAlgorithmProxy):
            # If we have an algorithm proxy, get the actual algorithm
            algorithm = algorithm._getHeldObject()
            
        if isinstance(algorithm, types.StringType):
            raise RuntimeError, "Use an algorithm function you moron"
        
        if isinstance(algorithm, types.StringType):
            # If we have a string, assume it's an algorithm name
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = None
                    self._data_file = None
                    if "filename" in kwargs:
                        self._data_file = kwargs["filename"]
                def get_algorithm(self):
                    return self.algorithm
                def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
                    """
                        Create a new instance of the requested algorithm object, 
                        set the algorithm properties replacing the input and output
                        workspaces.
                        The execution will work for any combination of mandatory/optional
                        properties. 
                        @param reducer: Reducer object managing the reduction
                        @param inputworkspace: input workspace name [optional]
                        @param outputworkspace: output workspace name [optional]
                    """
                    # If we don't have a data file, look up the workspace handle
                    if self._data_file is None:
                        if inputworkspace in reducer._data_files:
                            data_file = reducer._data_files[inputworkspace]
                            if data_file is None:
                                return
                        else:
                            raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
                    else:
                        data_file = self._data_file

                    proxy = MantidFramework.mtd._createAlgProxy(algorithm)
                    if not isinstance(proxy, MantidFramework.IAlgorithmProxy):
                        raise RuntimeError, "Reducer expects a ReductionStep or a function returning an IAlgorithmProxy object"                    
                    
                    propertyOrder = MantidFramework.mtd._getPropertyOrder(proxy._getHeldObject())
            
                    # add the args to the kw list so everything can be set in a single way
                    for (key, arg) in zip(propertyOrder[:len(args)], args):
                        kwargs[key] = arg

                    # Override input and output workspaces
                    if "Workspace" in propertyOrder:
                        algorithm.setPropertyValue("Workspace", MantidFramework._makeString(inputworkspace).lstrip('? '))
                    if "OutputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("OutputWorkspace", MantidFramework._makeString(inputworkspace).lstrip('? '))
                    if "Filename" in propertyOrder:
                        algorithm.setPropertyValue("Filename", data_file)

                    if "AlternateName" in kwargs and \
                        kwargs["AlternateName"] in propertyOrder:
                        algorithm.setPropertyValue(kwargs["AlternateName"], data_file)

                    # set the properties of the algorithm                    
                    ialg = proxy._getHeldObject()
                    self.algorithm = ialg
                    for key in kwargs.keys():
                        ialg.setPropertyValue(key, MantidFramework._makeString(kwargs[key]).lstrip('? '))
                        
                    proxy.execute()
            return f(reducer, _AlgorithmStep())
        
        elif isinstance(algorithm, MantidFramework.IAlgorithm) \
            or type(algorithm).__name__=="IAlgorithm":
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = algorithm
                    self._data_file = None
                    if "filename" in kwargs:
                        self._data_file = kwargs["filename"]
                def get_algorithm(self):
                    return self.algorithm
                def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
                    """
                        Create a new instance of the requested algorithm object, 
                        set the algorithm properties replacing the input and output
                        workspaces.
                        The execution will work for any combination of mandatory/optional
                        properties. 
                        @param reducer: Reducer object managing the reduction
                        @param inputworkspace: input workspace name [optional]
                        @param outputworkspace: output workspace name [optional]
                    """
                    # If we don't have a data file, look up the workspace handle
                    if self._data_file is None:
                        if inputworkspace in reducer._data_files:
                            data_file = reducer._data_files[inputworkspace]
                            if data_file is None:
                                return
                        else:
                            raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
                    else:
                        data_file = self._data_file
                                
                    propertyOrder = MantidFramework.mtd._getPropertyOrder(algorithm)
            
                    # Override input and output workspaces
                    if "Workspace" in propertyOrder:
                        algorithm.setPropertyValue("Workspace", MantidFramework._makeString(inputworkspace).lstrip('? '))
                    if "OutputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("OutputWorkspace", MantidFramework._makeString(inputworkspace).lstrip('? '))
                    if "Filename" in propertyOrder:
                        algorithm.setPropertyValue("Filename", data_file)

                    if "AlternateName" in kwargs and \
                        kwargs["AlternateName"] in propertyOrder:
                        algorithm.setPropertyValue(kwargs["AlternateName"], data_file)

                    algorithm.execute()
            return f(reducer, _AlgorithmStep())

        else:
            raise RuntimeError, "%s expects a ReductionStep object, found %s" % (f.__name__, algorithm.__class__)
    return validated_f
                 
def validate_step(f):
    """
        Decorator for Reducer methods that need a ReductionStep
        object as its first argument.
        
        Example:
            @validate_step
            def some_func(self, reduction_step):
                [...]
                
        Arguments to a Mantid algorithm function should be passed as arguments.
        Example:
            #Load("my_file.txt", "my_wksp") will become:
            reducer.some_func(Load, "my_file.txt", "my_wksp")
            
        InputWorkspace and OutputWorkspace arguments can be left as None
        if they are to be overwritten by the Reducer.
    """
    
    def validated_f(reducer, algorithm, *args, **kwargs):
        """
            Wrapper function around the function f.
            The function ensures that the algorithm parameter
            is a sub-class of ReductionStep
            @param algorithm: algorithm name, ReductionStep object, or Mantid algorithm function
        """
        if issubclass(algorithm.__class__, ReductionStep) or algorithm is None:
            # If we have a ReductionStep object, just use it.
            # "None" is allowed as an algorithm (usually tells the reducer to skip a step)
            return f(reducer, algorithm)
        
        if isinstance(algorithm, types.FunctionType):
            # If we get a function, assume its name is an algorithm name
            algorithm = algorithm.func_name

        if isinstance(algorithm, MantidFramework.IAlgorithmProxy):
            # If we have an algorithm proxy, get the actual algorithm
            algorithm = algorithm._getHeldObject()
            
        if isinstance(algorithm, types.StringType):
            # If we have a string, assume it's an algorithm name
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = None
                def get_algorithm(self):
                    return self.algorithm
                def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
                    """
                        Create a new instance of the requested algorithm object, 
                        set the algorithm properties replacing the input and output
                        workspaces.
                        The execution will work for any combination of mandatory/optional
                        properties. 
                        @param reducer: Reducer object managing the reduction
                        @param inputworkspace: input workspace name [optional]
                        @param outputworkspace: output workspace name [optional]
                    """
                    if outputworkspace is None:
                        outputworkspace = inputworkspace 
                    proxy = MantidFramework.mtd._createAlgProxy(algorithm)
                    if not isinstance(proxy, MantidFramework.IAlgorithmProxy):
                        raise RuntimeError, "Reducer expects a ReductionStep or a function returning an IAlgorithmProxy object"                    
                    
                    propertyOrder = MantidFramework.mtd._getPropertyOrder(proxy._getHeldObject())
            
                    # add the args to the kw list so everything can be set in a single way
                    for (key, arg) in zip(propertyOrder[:len(args)], args):
                        kwargs[key] = arg

                    # Override input and output workspaces
                    if "Workspace" in kwargs:
                        kwargs["Workspace"] = inputworkspace
                    if "InputWorkspace" in kwargs:
                        kwargs["InputWorkspace"] = inputworkspace
                    if "OutputWorkspace" in kwargs:
                        kwargs["OutputWorkspace"] = outputworkspace

                    # set the properties of the algorithm                    
                    ialg = proxy._getHeldObject()
                    self.algorithm = ialg
                    for key in kwargs.keys():
                        ialg.setPropertyValue(key, MantidFramework._makeString(kwargs[key]).lstrip('? '))
                        
                    proxy.execute()
            return f(reducer, _AlgorithmStep())
                    
        elif isinstance(algorithm, MantidFramework.IAlgorithm) \
            or type(algorithm).__name__=="IAlgorithm":
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = algorithm
                def get_algorithm(self):
                    return self.algorithm
                def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
                    """
                        Create a new instance of the requested algorithm object, 
                        set the algorithm properties replacing the input and output
                        workspaces.
                        The execution will work for any combination of mandatory/optional
                        properties. 
                        @param reducer: Reducer object managing the reduction
                        @param inputworkspace: input workspace name [optional]
                        @param outputworkspace: output workspace name [optional]
                    """
                    if outputworkspace is None:
                        outputworkspace = inputworkspace 
                    propertyOrder = MantidFramework.mtd._getPropertyOrder(algorithm)
            
                    # Override input and output workspaces
                    if "Workspace" in propertyOrder:
                        algorithm.setPropertyValue("Workspace", MantidFramework._makeString(inputworkspace).lstrip('? '))
                    if "InputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("InputWorkspace", MantidFramework._makeString(inputworkspace).lstrip('? '))
                    if "OutputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("OutputWorkspace", MantidFramework._makeString(outputworkspace).lstrip('? '))

                    algorithm.execute()
            return f(reducer, _AlgorithmStep())
                    
        elif False and isinstance(algorithm, types.FunctionType):
            # If the algorithm is a function, in which case
            # we expect it to produce an algorithm proxy
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algm = algorithm
                def execute(self, reducer, inputworkspace=None, outputworkspace=None): 
                    """
                        Create a new instance of the requested algorithm object, 
                        set the algorithm properties replacing the input and output
                        workspaces.
                        The execution will work for any combination of mandatory/optional
                        properties. 
                        @param reducer: Reducer object managing the reduction
                        @param inputworkspace: input workspace name [optional]
                        @param outputworkspace: output workspace name [optional]
                    """
                    if outputworkspace is None:
                        outputworkspace = inputworkspace 
                    kwargs['execute'] = False
                    proxy = self.algm(*args, **kwargs)
                    if not isinstance(proxy, MantidFramework.IAlgorithmProxy):
                        raise RuntimeError, "Reducer expects a ReductionStep or a function returning an IAlgorithmProxy object"                    
                    _algm = proxy._getHeldObject()
                    
                    # The inspect module has changed in python 2.6 
                    if sys.version_info[0]==2 and sys.version_info[1]<6:
                        argspec = inspect.getargspec(self.algm)[0] 
                    else:
                        argspec = inspect.getargspec(self.algm).args     
                    
                    # Go through provided arguments
                    if len(args)>len(argspec):
                        raise RuntimeError, "Could not get call signature for %s" % _algm.name()
                    
                    for i in range(len(args)):
                        if argspec[i] == "InputWorkspace":
                            _algm.setPropertyValue("InputWorkspace", inputworkspace)
                        elif argspec[i] == "OutputWorkspace":
                            _algm.setPropertyValue("OutputWorkspace", outputworkspace)                        
                        else:
                            _algm.setPropertyValue(argspec[i], args[i])
                    
                    # Go through keyword arguments
                    for key in kwargs:
                        if key not in proxy.keys():
                            continue                        
                        if key == "InputWorkspace":
                            _algm.setPropertyValue("InputWorkspace", inputworkspace)
                        elif key == "OutputWorkspace":
                            _algm.setPropertyValue("OutputWorkspace", outputworkspace)                        
                        else:
                            _algm.setPropertyValue(key, kwargs[key])
                    mantidsimple.execute_algorithm(proxy)
                    
                    return "%s\n\t%s" % (self.algm.__name__, self.algm.__doc__)
                    
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
        
    def clean_up(self):
        """
            Removes all workspace flagged as dirty, use when a reduction aborts with errors
        """
        for bad_data in self._dirty:
            if MantidFramework.mtd.workspaceExists(bad_data):
                mantidsimple.DeleteWorkspace(bad_data)
            else:
                MantidFramework.mtd.sendLogMessage('reducer: Could not access tainted workspace '+bad_data)
            
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
            TODO: this needs to be an ordered list
        """
        if data_file is None:
            if MantidFramework.mtd.workspaceExists(workspace):
                self._data_files[workspace] = None
                return
            else:
                raise RuntimeError, "Trying to append a data set without a file name or an existing workspace."
        if type(data_file)==list:
            for item in data_file:
                # Check that the file exists
                self._full_file_path(item)
            
            if workspace is None:
                # Use the first file to deternine the workspace name
                workspace = extract_workspace_name(data_file[0])
        else:
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
    
    def post_process(self):
        """
            Reduction steps to be executed after all data files have been
            processed.
        """ 
        pass
        
    def reduce(self):
        """
            Go through the list of reduction steps
        """
        t_0 = time.time()
        instrument_name = ''
        
        # Check that an instrument was specified
        if self.instrument is not None:
            instrument_name = self.instrument.name()
        
        # Log text
        self.log_text = "%s reduction - %s\n" % (instrument_name, time.ctime())

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
    
        self.log_text += "Reduction completed in %g sec\n" % (time.time()-t_0)
        log_path = os.path.join(self._data_path,"%s_reduction.log" % instrument_name)
        self.log_text += "Log saved to %s" % log_path
        
        # Write the log to file
        f = open(log_path, 'a')
        f.write(self.log_text)
        f.close()
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
    
