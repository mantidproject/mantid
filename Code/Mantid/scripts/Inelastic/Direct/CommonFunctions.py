import mantid
from mantid.simpleapi import *
from mantid import api
import os
import string


def create_resultname(run_number, prefix='', suffix=''):
    """Create a string based on the run number and optional prefix and
    suffix.
    """
    if type(run_number) is api._api.MatrixWorkspace:
        run_number = run_number.getRunNumber();
    elif type(run_number) is str:
        if run_number in mtd:
            pws = mtd[run_number];
            run_number = pws.getRunNumber();

    if type(run_number) == list:
        name = create_resultname(run_number[0], prefix, suffix)
    elif type(run_number) == int:
        name = prefix + '{0:0>#6d}_spe{1}'.format(run_number,suffix)
    else:
        name = os.path.basename(str(run_number))
        # Hack any instrument name off the front so the output is the same as if you give it a run number
        name = name.lstrip(string.ascii_letters)
        if (suffix is None):
            name = prefix + os.path.splitext(name)[0] + '_spe'
        else:
            name = prefix + os.path.splitext(name)[0] + '_spe' + suffix

    return name

def create_dataname(input):
    """This assumes some kind of filename input and creates a workspace
    from the basename of the full file path
    """
    return os.path.basename(input)

# Keeps track of loaded data files so that they can be clean up easily
_loaded_data = []

def clear_loaded_data():
    """Clears any previously loaded data workspaces
    """
    global _last_mono_file, _loaded_data
    _last_mono_file = None
    for data_ws in _loaded_data:
        DeleteWorkspace(data_ws)
    _loaded_data = []

def is_loaded(filename):
    """Returns True if the file is already loaded, false otherwise
    """
    global _loaded_files
    data_name =  create_dataname(filename)
    if data_name in _loaded_files:
        return True
    else:
        return False

def mark_as_loaded(filename):
    """Mark a file as loaded.
    """
    global _loaded_data
    data_name =  create_dataname(filename)
    if data_name not in _loaded_data:
        logger.notice("Marking %s as loaded." % filename)
        _loaded_data.append(data_name)

def load_runs(inst_name, runs, sum=True, calibration=None,load_with_workspace=False):
    """
    Loads a list of files, summing if the required.
    """
    if type(runs) == list:
        if len(runs) == 1:
            sum = False
        if sum == True:
            if len(runs) == 0: raise RuntimeError("load_runs was supplied an empty list.")
            result_ws = load_run(inst_name,runs[0])
            summed = 'summed-run-files'
            CloneWorkspace(InputWorkspace=result_ws,OutputWorkspace=summed)
            sum_files(summed, runs[1:])
            result_ws = mtd[summed]
            mark_as_loaded(summed)
            return result_ws
        else:
            loaded = []
            for r in runs:
                loaded.append(load_run(inst_name,r,calibration,False,load_with_workspace))
            if len(loaded) == 1:
                return loaded[0]
            else:
                return loaded
    else:
        # Try a single run
        return load_run(inst_name, runs, calibration,False,load_with_workspace)

def load_run(inst_name, run_number, calibration=None, force=False, load_with_workspace=False):
    """Loads run into the given workspace.

    If force is true then the file is loaded regardless of whether
    its workspace exists already.
    """
    # If a workspace with this name exists, then assume it is to be used in place of a file
    if str(run_number) in mtd:
        logger.notice("%s already loaded as workspace." % str(run_number))
        if type(run_number) == str:
            loaded_ws = mtd[run_number]
        else:
            loaded_ws = run_number
    else:
        # If it doesn't exists as a workspace assume we have to try and load a file
        if type(run_number) == int:
            filename = find_file(run_number)
        elif type(run_number) == list:
            raise TypeError('load_run() cannot handle run lists')
        else:
            # Check if it exists, else tell Mantid to try and
            # find it
            if os.path.exists(run_number):
                filename = run_number
            else:
                filename = find_file(run_number)
        # The output name
        output_name = create_dataname(filename)
        if (not force) and (output_name in mtd):
            logger.notice("%s already loaded" % filename)
            return mtd[output_name]

        args={};
        ext = os.path.splitext(filename)[1].lower();
        wrong_monitors_name = False;
        if ext.endswith("raw"):
            if load_with_workspace:
                args['LoadMonitors']='Include'
            else:
                args['LoadMonitors']='Separate'

        elif ext.endswith('nxs'):
            args['LoadMonitors'] = '1'

        loaded_ws = Load(Filename=filename, OutputWorkspace=output_name,**args)
        if isinstance(loaded_ws,tuple) and len(loaded_ws)>1:
            mon_ws = loaded_ws[1];
            loaded_ws=loaded_ws[0];

        logger.notice("Loaded %s" % filename)

    ######## Now we have the workspace
    apply_calibration(inst_name, loaded_ws, calibration)
    return loaded_ws

def apply_calibration(inst_name, loaded_ws, calibration):
    """
    """
    if loaded_ws.run().hasProperty("calibrated"):
        return

    if type(calibration) == str or type(calibration) == int:
        logger.debug('load_data: Moving detectors to positions specified in cal file "%s"' % str(calibration))
        filename = calibration
        skip_lines = None
        if type(filename) == int: # assume run number
            filename = inst_name + str(filename)
        # Pull in pressures, thicknesses & update from cal file
        LoadDetectorInfo(Workspace=loaded_ws, DataFilename=filename, RelocateDets=True)
        AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(calibration))
    elif isinstance(calibration, mantid.api.Workspace):
        logger.debug('load_data: Copying detectors positions from workspace "%s": ' % calibration.name())
        CopyInstrumentParameters(InputWorkspace=calibration,OutputWorkspace=loaded_ws)
        AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(calibration))

def sum_files(accumulator, files, file_type):
    """
    Sum a current workspace and a list of files, accumulating the results in the
    given workspace
    """
    if type(files) == list:
        for filename in files:
            temp = load_run(filename, file_type)
            Plus(LHSWorkspace=accumulator,RHSWorkspace=temp,OutputWorkspace=accumulator)
    else:
        pass

class switch(object):
    """ Helper class providing nice switch statement""" 
    def __init__(self, value):
        self.value = value
        self.fall = False

    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
        raise StopIteration
    
    def match(self, *args):
        """Indicate whether or not to enter a case suite"""
        if self.fall or not args:
            return True
        elif self.value in args: # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False