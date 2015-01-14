""" File contains Descriptors used describe run for direct inelastic reduction """ 


from mantid.simpleapi import *
from PropertiesDescriptors import *


class RunDescriptor(PropDescriptor):
    """ descriptor supporting a run and a workspace  """

    # the host class referencing contained all instantiated descriptors. 
    # Descriptors methods rely on it to work (e.g. to extract file loader preferences) 
    # so it has to be set up manually by PropertyManager __init__ method
    _holder=None
    _logger = None

    def __init__(self,ws_preffix,DocString=None): 
        """ """
        # Run number 
        self._run_number  = None
        # Extension of the file to load data from
        # 
        self._run_file_path = ''
        self._run_ext     = None
        # Workspace name which corresponds to the run 
        self._run_ws_name = None
        # String used to identify the workspace related to this property w.r.t. other workspaces
        self._ws_preffix  = ws_preffix
        #
        if not DocString is None:
            self.__doc__ = DocString

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
           self._run_number = value.getRunNumber()
           self._run_ws_name= value.name()
           return

       if isinstance(value,str): # it may be run number as string or it may be a workspace name
          if value in mtd: # workspace
              self._run_ws_name = value
              ws = mtd[value]
              self._run_number = ws.getRunNumber()
              return 
          else:
              run_num,file_path,fext = prop_helpers.parse_run_number_string(value) # TODO: parser  
              self._run_number = run_num
              if len(fext) > 0:
                  self._run_ext = fext
       elif isinstance(value,list):
           self._run_number = value
       else:
           self._run_number = int(value)

       self._run_ws_name = None
       self.get_ws_name()


    def get_file_ext(self):
        """ Method returns current file extension for file to load workspace from 
            e.g. .raw or .nxs extension
        """ 
        if self._run_ext:
            return self._run_ext
        else: # return IDF default
            return RunDescriptor._holder.data_file_ext

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

        inst_name = RunDescriptor._holder.short_inst_name
        if run_num:
            run_num_str = str(run_num)
        else:
            run_num_str = str(self.__get__(RunDescriptor._holder))
        #
        old_ext = self.get_file_ext()
        file_hint =inst_name + run_num_str + old_ext
        if len(self._run_file_path)>0:
            file_hint = self._run_file_path+file_hint
            if os.path.exists(file_hint):
                return file_hint

        try:
            file = FileFinder.findRuns(file_hint)[0]
            fname,fex=os.path.splitext(file)
            self._run_ext = fex
            if old_ext != fex:
                message='Cannot find file with extension {0} but find file {1} instead'.format(old_ext,file)
                RunDescriptor._logger(message,'notice')
            return file
        except RuntimeError:
             message = 'Cannot find file matching hint {0} on current search paths ' \
                       'for instrument {1}'.format(file_hint,inst_name)

             RunDescriptor._logger(message,'warning')
             return 'ERROR:find_file: '+message

    def get_workspace(self):
        """ Method returns workspace correspondent to current run number(s)
            and loads this workspace if it has not been loaded

            Returns Mantid pointer to the workspace, corresponding to this run number
        """ 
        if not self._run_ws_name:
           return None

        if self._run_ws_name in mtd:
           return mtd[self._run_ws_name]
        else:
           if self._run_number:
               inst_name   = RunDescriptor._holder.short_inst_name
               changed_prop = RunDescriptor._holder.getChangedProperties()
               if 'det_cal_file' in changed_prop:
                   use_workspace_calibration = False
               else:
                   use_workspace_calibration = True

               calibration = RunDescriptor._holder.det_cal_file
               return self.load_run(inst_name, calibration,False, RunDescriptor._holder.load_monitors_with_workspace,use_workspace_calibration)
           else:
              return None

    def get_monitors_ws(self):
        """ get pointer to a workspace containing monitors. 

           Explores different ways of finding monitor workspace in Mantid and returns the python pointer to the
           workspace which contains monitors.


        """

        data_ws = self.get_workspace()

        monWS_name = self.get_ws_name()+'_monitors'
        if monWS_name in mtd:
            mon_ws = mtd[monWS_name]
            monitors_separate = True
        else:
            mon_ws = data_ws
            monitors_separate = False

        spec_to_mon =   RunDescriptor._holder.spectra_to_monitors_list
        if monitors_separate and spec_to_mon :
            for specID in spec_to_mon:
                mon_ws=self.copy_spectrum2monitors(data_ws,mon_ws,specID);

        return mon_ws

    def get_ws_name(self):
        """ return workspace name. If ws name is not defined, build it first and set up as target ws name
        """ 

        if self._run_ws_name:
            return self._run_ws_name

        if RunDescriptor._holder:
            instr_name = RunDescriptor._holder.short_inst_name
        else:
            instr_name = '_test_instrument'

        suffix = '' # not implemented
        if RunDescriptor._holder.sum_runs:
            ws_name = "NotImplemented" 
        else:
            ws_name  =  '{0}{1}{2:0>#6d}{3}'.format(instr_name,self._ws_preffix,self._run_number,suffix)


        self._run_ws_name = ws_name

        return ws_name


    def load_run(self,inst_name, calibration=None, force=False, load_mon_with_workspace=False,use_ws_calibration=True):
        """Loads run into the given workspace.

           If force is true then the file is loaded regardless of whether  its workspace exists already
        """
        # If a workspace with this name exists, then assume it is to be used in place of a file
        ws_name = self.get_ws_name()

        if ws_name in mtd and not(force):
            RunDescriptor._logger("{0} already loaded as workspace.".format(self._run_ws_name),'information')
            loaded_ws = mtd[ws_name]
        else:
            # If it doesn't exists as a workspace assume we have to try and load a file
            data_file = self.find_file()
            if data_file[0:4] == 'ERROR':
                raise IOError(data_file)
                       
            format = self.get_file_ext()
            if load_mon_with_workspace:
                   mon_load_option = 'Include'
            else:
                   mon_load_option = 'Separate'
            #
            try: # Hack LoadEventNexus does not understand Separate at the moment and throws
                Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = mon_load_option)
            except ValueError:
                #mon_load_option =str(int(load_mon_with_workspace))
                Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = '1')


            RunDescriptor._logger("Loaded {0}".format(data_file),'information')
            loaded_ws = mtd[ws_name]

        ######## Now we have the workspace
        self.apply_calibration(loaded_ws,calibration,use_ws_calibration)
        loaded_ws = mtd[ws_name]
        return loaded_ws

    def apply_calibration(self,loaded_ws,calibration=None,use_ws_calibration=True):
        """  If calibration is present, apply it to the workspace 
        
             use_ws_calibration -- if true, retrieve workspace calibration option, stored with workspace and try to use it
        """

        if not (calibration or use_ws_calibration):
            return 
        if not isinstance(loaded_ws, api.Workspace):
           raise RuntimeError(' Calibration can be applied to a workspace only and got object of type {0}'.format(type(loaded_ws)))
        
        if loaded_ws.run().hasProperty("calibrated"):
            return # already calibrated

        ws_calibration = calibration
        if use_ws_calibration:
            try:
                ws_calibration = prop_helpers.get_default_parameter(loaded_ws.getInstrument(),'det_cal_file')
                if ws_calibration is None:
                    ws_calibration = calibration
                if isinstance(ws_calibration,str) and ws_calibration.lower() == 'none':
                    ws_calibration = calibration
                if ws_calibration :
                    test_name = ws_calibration
                    ws_calibration = FileFinder.getFullPath(ws_calibration)
                    if len(ws_calibration) == 0:
                       raise RuntimeError('Can not find defined in run {0} calibration file {1}\n'\
                                          'Define det_cal_file reduction parameter properly'.format(loaded_ws.name(),test_name))
                    RunDescriptor._logger('*** load_data: Calibrating data using workspace defined calibration file: {0}'.format(ws_calibration),'notice')
            except KeyError: # no det_cal_file defined in workspace
                if calibration:
                    ws_calibration = calibration
                else:
                    return

        if type(ws_calibration) == str : # It can be only a file (got it from calibration property)
            RunDescriptor._logger('load_data: Moving detectors to positions specified in cal file {0}'.format(ws_calibration),'debug')
            # Pull in pressures, thicknesses & update from cal file
            LoadDetectorInfo(Workspace=loaded_ws, DataFilename=ws_calibration, RelocateDets=True)
            AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(ws_calibration))
        elif isinstance(ws_calibration, api.Workspace):
            RunDescriptor._logger('load_data: Copying detectors positions from workspace {0}: '.format(ws_calibration.name()),'debug')
            CopyInstrumentParameters(InputWorkspace=ws_calibration,OutputWorkspace=loaded_ws)
            AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(ws_calibration))

    @staticmethod
    def copy_spectrum2monitors(data_ws,mon_ws,spectraID):
       """
        this routine copies a spectrum form workspace to monitor workspace and rebins it according to monitor workspace binning

        @param data_ws  -- the  event workspace which detector is considered as monitor or Mantid pointer to this workspace
        @param mon_ws   -- the  histogram workspace with monitors where one needs to place the detector's spectra 
        @param spectraID-- the ID of the spectra to copy.

       """
 
       # ----------------------------
       try:
           ws_index = mon_ws.getIndexFromSpectrumNumber(spectraID)
           # Spectra is already in the monitor workspace
           return mon_ws
       except:
           ws_index = data_ws.getIndexFromSpectrumNumber(spectraID)

       #
       x_param = mon_ws.readX(0);
       bins = [x_param[0],x_param[1]-x_param[0],x_param[-1]];
       ExtractSingleSpectrum(InputWorkspace=data_ws,OutputWorkspace='tmp_mon',WorkspaceIndex=ws_index)
       Rebin(InputWorkspace='tmp_mon',OutputWorkspace='tmp_mon',Params=bins,PreserveEvents='0')
       # should be vice versa but Conjoin invalidate ws pointers and hopefully nothing could happen with workspace during conjoining
       #AddSampleLog(Workspace=monWS,LogName=done_log_name,LogText=str(ws_index),LogType='Number')
       mon_ws_name = mon_ws.getName();
       ConjoinWorkspaces(InputWorkspace1=mon_ws,InputWorkspace2='tmp_mon')
       mon_ws =mtd[mon_ws_name]

       if 'tmp_mon' in mtd:
           DeleteWorkspace(WorkspaceName='tmp_mon')
       return mon_ws

  
#-------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorDependent(RunDescriptor):
    """ A RunDescriptor class dependent on another RunDescriptor"""

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