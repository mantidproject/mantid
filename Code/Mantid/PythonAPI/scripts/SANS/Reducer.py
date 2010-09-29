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
import SANSInsts

## Version number
__version__ = '0.0'
    
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
    ## List of reduction steps
    _reduction_steps = []
        
    def __init__(self):
        self._data_files = {}
        self._reduction_steps = []
        
    def set_instrument(self, configuration):
        if issubclass(configuration.__class__, SANSInsts.Instrument):
            self.instrument = configuration
        else:
            raise RuntimeError, "SANSReducer.set_instrument expects an Instrument object, found %s" % configuration.__class__
        
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
        else:
            # If not, raise an exception
            raise RuntimeError, "Reducer expects a file name in %s" % self._data_path
        
    def append_step(self, reduction_step):
        """
            Append a reduction step
            @param reduction_step: ReductionStep object
        """
        if issubclass(reduction_step.__class__, ReductionStep):
            self._reduction_steps.append(reduction_step)
        else:
            raise RuntimeError, "Reducer.append_step expects an object of class ReductionStep"
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
        # Check that an instrument was specified
        if self.instrument is None:
            raise RuntimeError, "Reducer: trying to run a reduction with an instrument specified"

        # Go through the list of steps that are common to all data files
        self.pre_process()

        # Go through the list of files to be reduced
        for file_ws in self._data_files:
            for item in self._reduction_steps:
                item.execute(self, file_ws)        

        #any clean up, possibly removing workspaces 
        self.post_process()
    
    
class ReductionStep(object):
    """
        Base class for reduction steps
    """ 
    def execute(self, reducer, workspace): 
        """
            Implemented the reduction step.
            @param reducer: Reducer object for which the step is executed
            @param workspace: Name of the workspace to apply this step to
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
    