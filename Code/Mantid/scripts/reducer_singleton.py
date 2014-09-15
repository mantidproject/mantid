import random
import string
import os
import mantid

from isis_instrument import BaseInstrument


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
    ## List of reduction steps
    _reduction_steps = []
    ## Log
    log_text = ''
    ## Output workspaces
    output_workspaces = []

    def __init__(self):
        self.UID = ''.join(random.choice(string.ascii_lowercase + string.ascii_uppercase + string.digits) for x in range(5))
        self._reduction_steps = []

    def set_instrument(self, configuration):
        if issubclass(configuration.__class__, BaseInstrument):
            self.instrument = configuration
        else:
            raise RuntimeError, "Reducer.set_instrument expects an %s object, found %s" % (Instrument, configuration.__class__)

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
        # should we use it?
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
        #for file_ws in self._data_files:
        #    for item in self._reduction_steps:
        #        try:
        #            result = item.execute(self, file_ws)
        #            if result is not None and len(str(result))>0:
        #                self.log_text += "%s\n" % str(result)
        #        except:
        #            self.log_text += "\n%s\n" % sys.exc_value
        #            raise

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
    def replace(cls, red):
        """
            Set the object pointed to by the singleton with
            the one passed
            @param red: reducer object
        """
        if issubclass(red.__class__, Reducer):
            ReductionSingleton.__instance = red
        else:
            raise RuntimeError, 'The object passed to ReductionSingleton.replace() must be of type Reducer'


    @classmethod
    def run(cls):
        """
            Execute the reducer and then clean it (regardless of
            if it throws) to ensure that a partially run reducer is
            not left behind
        """
        try:
            if ReductionSingleton.__instance is not None:
                return ReductionSingleton.__instance._reduce()
        finally:
            ReductionSingleton.clean(ReductionSingleton.__instance.__class__)

    def __getattr__(self, attr):
        """ Delegate access to implementation """
        return getattr(self.__instance, attr)

    def __setattr__(self, attr, value):
        """ Delegate access to implementation """
        return setattr(self.__instance, attr, value)

