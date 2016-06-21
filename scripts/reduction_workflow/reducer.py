#pylint: disable=invalid-name
"""
    Base reduction class. Uses version 2 python API.
"""
import os
import sys
import time
import random
import string
from mantid.api import AlgorithmManager, AnalysisDataService
from mantid.kernel import ConfigService, Logger

class Reducer(object):
    """
        Base reducer class. Instrument-specific reduction processes should be
        implemented in a child of this class.
    """
    ## Path for data files
    _data_path = '.'
    ## Path for output files
    _output_path = None
    ## List of data files to process
    _data_files = {}
    ## List of reduction steps
    _reduction_steps = []
    ## Log
    log_text = ''
    ## Output workspaces
    output_workspaces = []
    reduction_properties = {}
    instrument_name = ""
    setup_algorithm = None
    reduction_algorithm = None

    def __init__(self):
        self.UID = ''.join(random.choice(string.ascii_lowercase + string.ascii_uppercase + string.digits) for x in range(5))
        self.property_manager = "__reduction_parameters_"+self.UID
        self._data_files = {}
        self._reduction_steps = []
        self.reduction_properties = {}

    def get_reduction_table_name(self):
        return self.property_manager

    def set_reduction_table_name(self, name):
        self.property_manager = str(name)

    def set_instrument(self, instrument, setup_algorithm=None,
                       reduction_algorithm=None):
        self.instrument_name = instrument
        self.setup_algorithm = setup_algorithm
        self.reduction_algorithm = reduction_algorithm

    def get_instrument(self):
        return self.instrument_name

    def set_data_path(self, path):
        """
            Set the path for data files
            @param path: data file path
        """
        path = os.path.normcase(path)
        if os.path.isdir(path):
            self._data_path = path
            ConfigService.Instance().appendDataSearchDir(path)
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
            if AnalysisDataService.doesExist(workspace):
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
        Logger("Reducer").information("Setting up reduction options")
        if self.setup_algorithm is not None:
            alg = AlgorithmManager.create(self.setup_algorithm)
            alg.initialize()
            props = [p.name for p in alg.getProperties()]
            for key in self.reduction_properties.keys():
                if key in props:
                    try:
                        alg.setProperty(key, self.reduction_properties[key])
                    except:
                        msg = "Error setting %s=%s" % (key, str(self.reduction_properties[key]))
                        msg += "\n  %s" % sys.exc_value
                        Logger("Reducer").error(msg)
                else:
                    Logger("Reducer").warning("Setup algorithm has no %s property" % key)

            if "ReductionProperties" in props:
                alg.setPropertyValue("ReductionProperties",
                                     self.get_reduction_table_name())
            alg.execute()

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
        self.output_workspaces = []

        # Log text
        self.log_text = "%s reduction - %s\n" % (self.instrument_name, time.ctime())
        self.log_text += "Mantid Python API v2\n"

        # Go through the list of steps that are common to all data files
        self.pre_process()

        if self.reduction_algorithm is None:
            Logger("Reducer").error("A reduction algorithm wasn't set: stopping")
            return

        _first_ws_name = None
        for ws in self._data_files.keys():
            if _first_ws_name is None:
                _first_ws_name = ws
            alg = AlgorithmManager.create(self.reduction_algorithm)
            alg.initialize()
            props = [p.name for p in alg.getProperties()]

            # Check whether the data is already available or needs to be loaded
            if self._data_files[ws] is not None:
                datafile = self._data_files[ws]
                if type(datafile)==list:
                    datafile=','.join(datafile)
                if "Filename" in props:
                    alg.setPropertyValue("Filename", datafile)
                else:
                    msg = "Can't set the Filename property on %s" % self.reduction_algorithm
                    Logger("Reducer").error(msg)
            else:
                if "InputWorkspace" in props:
                    alg.setPropertyValue("InputWorkspace", ws)
                else:
                    msg = "Can't set the InputWorkspace property on %s" % self.reduction_algorithm
                    Logger("Reducer").error(msg)

            if "ReductionProperties" in props:
                alg.setPropertyValue("ReductionProperties",
                                     self.get_reduction_table_name())

            if "OutputWorkspace" in props:
                alg.setPropertyValue("OutputWorkspace", ws)

            alg.execute()
            if "OutputMessage" in props:
                self.log_text += alg.getProperty("OutputMessage").value+'\n'

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
        if _first_ws_name is not None:
            log_path = os.path.join(output_dir, "%s_reduction.log" % _first_ws_name)
        else:
            log_path = os.path.join(output_dir,"%s_reduction.log" % self.instrument_name)
        self.log_text += "Log saved to %s" % log_path

        # Write the log to file
        f = open(log_path, 'a')
        f.write("\n-------------------------------------------\n")
        f.write(self.log_text)
        f.close()
        return self.log_text

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

