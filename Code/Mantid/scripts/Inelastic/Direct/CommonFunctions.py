import mantid
from mantid.simpleapi import *
from mantid import api
import os
import string


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