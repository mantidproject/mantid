""" File contains Descriptors used describe run for direct inelastic reduction """ 


from mantid.simpleapi import *
from PropertiesDescriptors import *


class RunDescriptor(PropDescriptor):
    """ descriptor supporting a run and a workspace  """

    # the host class referencing contained all instantiated descriptors. 
    # Descriptors methods rely on it to work (e.g. to extract file loader preferences) 
    # so it has to be set up manually by PropertyManager __init__ method
    __holder_class__=None
    logger = None

    def __init__(self,ws_preffix,DocString=None): 
        """ """
        # Run number 
        self._run_number  = None
        # Extension of the file to load data from
        self._run_ext     = None
        # Workspace name which corresponds to the run 
        self._run_ws_name = None
        # String used to identify the workspace related to this property w.r.t. other workspaces
        self._ws_preffix  = ws_preffix
        #
        if not DocString is None:
            self.__doc__ = DocString

        if RunDescriptor.__holder_class__:
            logger = RunDescriptor.__holder_class__.log()

    def __get__(self,instance,owner=None):
        """ return current run number""" 
        if instance is None:
           return self
        return self._run_number 

    def __set__(self,instance,value):
       """ Set up Run number from any source """
       if value == None: # clear current run number
           self._run_number = None
           self._run_ws_name = None
           self._run_ext     = None
           return
       if isinstance(value, api.Workspace):
           self._run_number = value.getRunNuber()
           self._run_ws_name= value.name()
           return

       if isinstance(value,str): # it may be run number as string or it may be a workspace name
          if value in mtd: # workspace
              self._run_ws_name = value
              ws = mtd[value]
              self._run_number = ws.getRunNumber()
              return 
          else:
              self._run_number = parse_run_number_string(value) # TODO: parser      
       elif isinstance(value,list):
           self._run_number = value
       else:
           self._run_number = int(value)

       self._run_ws_name = None
       self._run_ws_name = self.get_ws_name()


    def get_file_ext(self):
        """ Method returns current file extension for file to load workspace from 
            e.g. .raw or .nxs extension
        """ 
        if self._run_ext:
            return self._run_ext
        else: # return IDF default
            return RunDescriptor.__holder_class__.data_file_ext

    def set_file_ext(self,val):
        """ set non-default file extension """
        if isinstance(val,str):
            if val[0] != '.':
                value = '.' + val
            else:
                value = val
            self._run_ext = value
        else:
            raise AttributeError('Source file extension can be only a string')

    def find_file(self,run_num = None):
        """Use Mantid to search for the given run. """

        inst_name = RunDescriptor.__holder_class__.short_inst_name
        if run_num:
            run_num_str = str(run_num)
        else:
            run_num_str = str(self.__get__(RunDescriptor.__holder_class__))
        #
        file_hint =inst_name + run_num_str + self.get_file_ext()
        try:
            return FileFinder.findRuns(file_hint)[0]
        except RuntimeError:
            message = 'Cannot find file matching hint {0} on current search paths ' \
                      'for instrument {1}'.format(file_hint,inst_name)

            logger(message,'warning')
            return 'ERROR:find_file '+message

    def get_workspace(self):
        """ Method returns workspace correspondent to current run number(s)
            and loads this workspace if necessary 
        """ 
        if not self._run_ws_name:
           return None

        if self._run_ws_name in mtd:
           return mtd[self._run_ws_name]
        else:
           if self._run_number:
               inst_name   = RunDescriptor.__holder_class__.short_inst_name
               calibration = RunDescriptor.__holder_class__.det_cal_file
               return self.load_run(inst_name, calibration,False, RunDescriptor.__holder_class__.load_monitors_with_workspace)
           else:
              return None

    def get_ws_name(self):
        """ return workspace name. If ws name is not defined, build it first and set up as target ws name

        """ 

        if self._run_ws_name:
            return self._run_ws_name

        if RunDescriptor.__holder_class__:
            instr_name = RunDescriptor.__holder_class__.short_inst_name
        else:
            instr_name = '_test_instrument'

        if not RunDescriptor.__holder_class__.sum_runs:
            ws_name = common.create_resultname(self._run_number,instr_name+self._ws_preffix)
        else:
            ws_name = common.create_resultname(self._run_number,instr_name+self._ws_preffix,'-sum')
        self._run_ws_name = ws_name

        return ws_name


    def load_run(self,inst_name, calibration=None, force=False, load_with_workspace=False):
        """Loads run into the given workspace.

           If force is true then the file is loaded regardless of whether  its workspace exists already
        """
        # If a workspace with this name exists, then assume it is to be used in place of a file
        ws_name = self.get_ws_name()

        if ws_name in mtd and not(force):
            RunDescriptor.logger("{0} already loaded as workspace.".format(self._run_ws_name),'notice')
            loaded_ws = mtd[ws_name]
        else:
            # If it doesn't exists as a workspace assume we have to try and load a file
            data_file = self.find_file()
            if data_file[0:4] == 'ERROR':
                raise IOError(data_file)           


            Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = str(int(load_with_workspace)))
            RunDescriptor.logger("Loaded {0}".format(data_file),'notice')
            loaded_ws = mtd[ws_name]

        ######## Now we have the workspace
        self.apply_calibration(loaded_ws,calibration)
        return loaded_ws

    def apply_calibration(self,loaded_ws,calibration=None):
        """  If calibration is present, apply it to the workspace """

        if not calibration:
            return
        if not isinstance(loaded_ws, api.Workspace):
           raise RuntimeError(' Calibration can be applied to a workspace only and got object of type {0}'.format(type(loaded_ws)))
        
        if loaded_ws.run().hasProperty("calibrated"):
            return # already calibrated

        if type(calibration) == str : # It can be only a file (got it from calibration property)
            RunDescriptor.logger('load_data: Moving detectors to positions specified in cal file {0}'.format(calibration),'debug')
            # Pull in pressures, thicknesses & update from cal file
            LoadDetectorInfo(Workspace=loaded_ws, DataFilename=calibration, RelocateDets=True)
            AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(calibration))
        elif isinstance(calibration, api.Workspace):
            logger('load_data: Copying detectors positions from workspace {0}: '.format(calibration.name()),'debug')
            CopyInstrumentParameters(InputWorkspace=calibration,OutputWorkspace=loaded_ws)
            AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(calibration))
  
#-------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorDependent(RunDescriptor):
    def __init__(self,host_run,ws_preffix,DocString=None):
        RunDescriptor.__init__(self,ws_preffix,DocString)
        self._host = host_run
        self._this_run_defined=False

    def __get__(self,instance,owner=None):
       """ return dependent run number which is host run number if this one has not been set or this run number if it was""" 
       if instance is None:
           return self
       if self._this_run_defined:
          return self._run_number
       else:
          return self._host.__get__(instance,owner)

    def __set__(self,instance,value):
        if value is None:
            self._this_run_defined = False
            return
        self._this_run_defined = True
        super(RunDescriptorDependent,self).__set__(instance,value)

    def get_workspace(self):
        """ overloaded get workspace method """ 
        if self._this_run_defined:
            self.get_workspace()
        else:
            self._host.get_workspace()