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
import mantid
from mantid import simpleapi
import warnings
import inspect
import random
import string
from find_data import find_data


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

        if isinstance(algorithm, types.StringType):
            # If we have a string, assume it's an algorithm name
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = None
                    self._data_file = None
                def get_algorithm(self):
                    return self.algorithm
                def setProperty(self, key, value):
                    kwargs[key]=value
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

                    alg = mantid.api.AlgorithmManager.create(algorithm)
                    if not isinstance(alg, mantid.api.AlgorithmProxy):
                        raise RuntimeError, "Reducer expects an Algorithm object from FrameworkManager, found '%s'" % str(type(alg))

                    propertyOrder = alg.orderedProperties()

                    # add the args to the kw list so everything can be set in a single way
                    for (key, arg) in zip(propertyOrder[:len(args)], args):
                        kwargs[key] = arg

                    # Override input and output workspaces
                    if "Workspace" in kwargs:
                        kwargs["Workspace"] = inputworkspace
                    if "OutputWorkspace" in kwargs:
                        kwargs["OutputWorkspace"] = inputworkspace
                    if "Filename" in kwargs:
                        kwargs["Filename"] = data_file

                    if "AlternateName" in kwargs and \
                        kwargs["AlternateName"] in propertyOrder:
                        kwargs[kwargs["AlternateName"]] = data_file

                    self.algorithm = alg
                    simpleapi._set_properties(alg, *(), **kwargs)
                    alg.execute()
                    if "OutputMessage" in propertyOrder:
                        return alg.getPropertyValue("OutputMessage")
                    return "%s applied" % alg.name()
            return f(reducer, _AlgorithmStep())

        elif isinstance(algorithm, mantid.api.IAlgorithm) \
            or type(algorithm).__name__=="IAlgorithm":
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = algorithm
                    self._data_file = None
                def get_algorithm(self):
                    return self.algorithm
                def setProperty(self, key, value):
                    kwargs[key]=value
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

                    propertyOrder = algorithm.orderedProperties()

                    # Override input and output workspaces
                    if "Workspace" in propertyOrder:
                        algorithm.setPropertyValue("Workspace", inputworkspace)
                    if "OutputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("OutputWorkspace", inputworkspace)
                    if "Filename" in propertyOrder:
                        algorithm.setPropertyValue("Filename", data_file)

                    if "AlternateName" in kwargs and \
                        kwargs["AlternateName"] in propertyOrder:
                        algorithm.setPropertyValue(kwargs["AlternateName"], data_file)

                    algorithm.execute()
                    return "%s applied" % algorithm.name()
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

        if isinstance(algorithm, types.StringType):
            # If we have a string, assume it's an algorithm name
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = None
                def get_algorithm(self):
                    return self.algorithm
                def setProperty(self, key, value):
                    kwargs[key]=value
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
                    alg = mantid.AlgorithmManager.create(algorithm)
                    if not isinstance(alg, mantid.api.AlgorithmProxy):
                        raise RuntimeError, "Reducer expects an Algorithm object from FrameworkManager, found '%s'" % str(type(alg))

                    propertyOrder = alg.orderedProperties()

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

                    self.algorithm = alg
                    simpleapi._set_properties(alg,*(),**kwargs)
                    alg.execute()
                    if "OutputMessage" in propertyOrder:
                        return alg.getPropertyValue("OutputMessage")
                    return "%s applied" % alg.name()
            return f(reducer, _AlgorithmStep())

        elif isinstance(algorithm, mantid.api.IAlgorithm) \
            or type(algorithm).__name__=="IAlgorithm":
            class _AlgorithmStep(ReductionStep):
                def __init__(self):
                    self.algorithm = algorithm
                def get_algorithm(self):
                    return self.algorithm
                def setProperty(self, key, value):
                    kwargs[key]=value
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
                    propertyOrder = algorithm.orderedProperties()

                    # Override input and output workspaces
                    if "Workspace" in propertyOrder:
                        algorithm.setPropertyValue("Workspace", inputworkspace)
                    if "InputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("InputWorkspace", inputworkspace)
                    if "OutputWorkspace" in propertyOrder:
                        algorithm.setPropertyValue("OutputWorkspace", outputworkspace)

                    algorithm.execute()
                    if "OutputMessage" in propertyOrder:
                        return algorithm.getPropertyValue("OutputMessage")
                    return "%s applied" % algorithm.name()
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
    ## Path for output files
    _output_path = None
    ## List of data files to process
    _data_files = {}
    ## List of workspaces that were modified
    _dirty = []
    ## List of reduction steps
    _reduction_steps = []
    ## Log
    log_text = ''
    ## Output workspaces
    output_workspaces = []

    def __init__(self):
        self.UID = ''.join(random.choice(string.ascii_lowercase + string.ascii_uppercase + string.digits) for x in range(5))
        self.property_manager = "__reduction_parameters_"+self.UID
        self._data_files = {}
        self._reduction_steps = []

    def get_reduction_table_name(self):
        return self.property_manager

    def set_reduction_table_name(self, name):
        self.property_manager = str(name)

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
            if bad_data in mtd:
                simpleapi.DeleteWorkspace(Workspace=bad_data)
            else:
                mantid.logger.notice('reducer: Could not access tainted workspace '+bad_data)

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
        path = os.path.normcase(path)
        if os.path.isdir(path):
            self._data_path = path
            mantid.config.appendDataSearchDir(path)
        else:
            raise RuntimeError, "Reducer.set_data_path: provided path is not a directory (%s)" % path

    def set_output_path(self, path):
        """
            Set the path for output files
            @param path: output file path
        """
        path = os.path.normcase(path)
        if os.path.isdir(path):
            self._output_path = path
        else:
            raise RuntimeError, "Reducer.set_output_path: provided path is not a directory (%s)" % path

    def _full_file_path(self, filename):
        """
            Prepends the data folder path and returns a full path to the given file.
            Raises an exception if the file doesn't exist.
            @param filename: name of the file to create the full path for
        """
        lineno = inspect.currentframe().f_code.co_firstlineno
        warnings.warn_explicit("Reducer._full_file_path is deprecated: use find_data instead", DeprecationWarning, __file__, lineno)

        instrument_name = ''
        if self.instrument is not None:
            instrument_name = self.instrument.name()

        return find_data(filename, instrument=instrument_name)

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

    def clear_data_files(self):
        """
            Empty the list of files to reduce while keeping all the
            other options the same.
        """
        self._data_files = {}
    def append_data_file(self, data_file, workspace=None):
        """
            Append a file to be processed.
            @param data_file: name of the file to be processed
            @param workspace: optional name of the workspace for this data,
                default will be the name of the file
            TODO: this needs to be an ordered list
        """
        if data_file is None:
            if workspace in mtd:
                self._data_files[workspace] = None
                return
            else:
                raise RuntimeError, "Trying to append a data set without a file name or an existing workspace."
        if type(data_file)==list:
            if workspace is None:
                # Use the first file to determine the workspace name
                workspace = extract_workspace_name(data_file[0])
        else:
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
        self.output_workspaces = []

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
                try:
                    result = item.execute(self, file_ws)
                    if result is not None and len(str(result))>0:
                        self.log_text += "%s\n" % str(result)
                except:
                    self.log_text += "\n%s\n" % sys.exc_value
                    raise

        #any clean up, possibly removing workspaces
        self.post_process()

        # Determine which directory to use
        output_dir = self._data_path
        if self._output_path is not None:
            if os.path.isdir(self._output_path):
                output_dir = self._output_path
            else:
                output_dir = os.path.expanduser('~')

        self.log_text += "Reduction completed in %g sec\n" % (time.time()-t_0)
        log_path = os.path.join(output_dir,"%s_reduction.log" % instrument_name)
        self.log_text += "Log saved to %s" % log_path

        # Write the log to file
        f = open(log_path, 'a')
        f.write("\n-------------------------------------------\n")
        f.write(self.log_text)
        f.close()
        return self.log_text


class ReductionStep(object):
    """
        Base class for reduction steps
    """
    @classmethod
    def delete_workspaces(cls, workspace):
        """
            Delete all workspace created by this reduction step related
            to the given workspace
            @param workspace: workspace to delete
        """
        return

    @classmethod
    def _create_unique_name(cls, filepath, descriptor):
        """
            Generate a unique name for an internal workspace
        """
        random_str = ''.join(random.choice(string.ascii_lowercase + string.ascii_uppercase + string.digits) for x in range(5))
        return "__"+descriptor+"_"+extract_workspace_name(filepath)+"_"+random_str

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
    filepath_tmp = filepath
    if type(filepath)==list:
        filepath_tmp = filepath[0]

    (head, tail) = os.path.split(filepath_tmp)
    basename, extension = os.path.splitext(tail)

    if type(filepath)==list:
        basename += "_combined"

    #TODO: check whether the workspace name is already in use
    #      and modify it if it is.

    return basename+suffix

