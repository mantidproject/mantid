#pylint: disable=invalid-name
""" File contains Descriptors used to describe run for direct inelastic reduction """


from mantid.simpleapi import *
from Direct.PropertiesDescriptors import *
import re


class RunDescriptor(PropDescriptor):
    """ descriptor supporting a run and a workspace  """

    # the host class referencing contained all instantiated descriptors.
    # Descriptors methods rely on it to work (e.g.  to extract file loader
    # preferences)
    # so it has to be set up manually by PropertyManager __init__ method
    _holder = None
    _logger = None
    # class level reference to the property manager
    _PropMan = None
#--------------------------------------------------------------------------------------------------------------------
    def __init__(self,prop_name,DocString=None): 
        """ """
        self._prop_name = prop_name
        if not DocString is None:
            self.__doc__ = DocString

        self._clear_all()
#--------------------------------------------------------------------------------------------------------------------
    def _clear_all(self):
        """ clear all internal properties and settings """ 
        # Run number
        self._run_number = None
        # Extension of the file to load data from
        #

        self._run_file_path = ''
        self._run_ext = None
        # Workspace name which corresponds to the run
        self._ws_name = None # none if not loaded
        # String used to identify the workspace related to this property w.r.t.
                                    # other workspaces
        self._ws_cname = ''
        self._ws_suffix = ''
        self._bind_to_sum = False

        #
        self._in_cash = False

#--------------------------------------------------------------------------------------------------------------------
    def __get__(self,instance,owner):
       """ return current run number or workspace if it is loaded""" 
       if not RunDescriptor._PropMan:
          RunDescriptor._PropMan = owner
       if instance is None:
           return self

       if self._ws_name and self._ws_name in mtd:
           return mtd[self._ws_name]
       else:
           return self._run_number 
#--------------------------------------------------------------------------------------------------------------------
    def __set__(self,instance,value):
       """ Set up Run number and define workspace name from any source """
       #
       if not RunDescriptor._PropMan:
          from PropertyManager import PropertyManager
          RunDescriptor._PropMan = PropertyManager

       old_ws_name = self._ws_name
       clear_fext = True
       #end
       if value == None: # clear current run number
           self._clear_all()
           self._clear_old_ws(old_ws_name,self._ws_name,clear_fext)
           RunDescriptor._PropMan.sum_runs.clear_sum()
           return
       if isinstance(value, api.Workspace):
           #if 'SumOfRuns:' in value.getRun():
               # TODO: not implemented

           #else:
           if self._ws_name != value.name():
                self._set_ws_as_source(value)
                self._clear_old_ws(old_ws_name,self._ws_name,clear_fext)
                self._bind_to_sum = False
                self._in_cash = False
                RunDescriptor._PropMan.sum_runs.clear_sum()
                return
           else: # it is just reassigning the same workspace to itself
             return

       if isinstance(value,str): # it may be run number as string or it may be a workspace name
          if value in mtd: # workspace name
              ws = mtd[value]
              self.__set__(instance,ws)
              return
          else:
              file_path,run_num,fext = prop_helpers.parse_run_file_name(value)

              if isinstance(run_num,list):
                  RunDescriptor._PropMan.sum_runs.set_list2add(run_num,file_path,fext)
                  self._bind_to_sum = True
                  if instance.sum_runs:
                     last_run_ind = RunDescriptor._PropMan.sum_runs.get_last_ind2sum()
                     main_fext = fext[last_run_ind].lower()
                     self._run_file_path = file_path[last_run_ind].lower()
              else:
                  self.__set__(instance,run_num)
                  self._run_file_path = file_path
                  main_fext = fext.lower()
              #
              if len(main_fext) > 0:
                 self._run_ext = main_fext
              else: # will be default file extension
                 self._run_ext = None
              clear_fext = False
              self._in_cash=False
       elif isinstance(value,list):
           if len(value) == 1:
               self.__set__(instance,value[0])
               return
           self._bind_to_sum = True
           RunDescriptor._PropMan.sum_runs.set_list2add(value)
           if instance.sum_runs:
                last_run_ind = RunDescriptor._PropMan.sum_runs.get_last_ind2sum()
                self._run_number = value[last_run_ind]
           else:
               self._run_number = value[0]
           clear_fext = True
       else:
           clear_fext = True
           self._run_number = int(value)
           if self._bind_to_sum and instance and instance.sum_runs:
              num2_sum = RunDescriptor._PropMan.sum_runs.set_last_ind2sum(self._run_number)                     
              if num2_sum == 0:
                self._bind_to_sum = False
                instance.sum_runs = False


       self._ws_cname = ''
       self._ws_name = None
       self._in_cash = False
       self._clear_old_ws(old_ws_name,None,clear_fext)
#--------------------------------------------------------------------------------------------------------------------
    def run_number(self):
        """ Return run number regardless of workspace is loaded or not"""
        if self._ws_name and self._ws_name in mtd:
            ws = mtd[self._ws_name]
            return ws.getRunNumber()
        else:
            return self._run_number 
#--------------------------------------------------------------------------------------------------------------------
    def is_monws_separate(self):
        """ """
        mon_ws = self.get_monitors_ws()
        if mon_ws:
            name = mon_ws.name()
        else:
            return False

        if name.endswith('_monitors'):
            return True
        else:
            return False
#--------------------------------------------------------------------------------------------------------------------
    def get_run_list(self):
        """ Returns list of the files, assigned to current property """
        current_run = self.run_number()
        if self._bind_to_sum:
            runs = RunDescriptor._PropMan.sum_runs.get_runs()
            if current_run in runs:
                return runs
            else:
                return [current_run]
        else:
           return [current_run]
#--------------------------------------------------------------------------------------------------------------------
    def set_action_suffix(self,suffix=None):
        """ method to set part of the workspace name, which indicate some action performed over this workspace           
            
            e.g.: default suffix of a loaded workspace is 'RAW' but we can set it to SPE to show that conversion to 
            energy will be performed for this workspace.

            method returns the name of the workspace is will have with this suffix. Algorithms would later  
            work on the initial workspace and modify it in-place or to produce workspace with new name (depending if one 
            wants to keep initial workspace)
            synchronize_ws(ws_pointer) then should synchronize workspace and its name.

            TODO: This method should be automatically invoked by an algorithm decorator
            Until implemented, one have to ensure that it is correctly used together with synchronize_ws
            to ensue one can always get workspace from its name
        """
        if suffix:
            self._ws_suffix = suffix
        else: # return to default
            self._ws_suffix = ''
        return self._build_ws_name()
#--------------------------------------------------------------------------------------------------------------------
    def synchronize_ws(self,workspace=None):
        """ Synchronize workspace name (after workspace may have changed due to algorithm) 
            with internal run holder name. Accounts for the situation when 

            TODO: This method should be automatically invoked by an algorithm decorator
            Until implemented, one have to ensure that it is correctly used together with 
            set_action_suffix to ensue one can always get expected workspace from its name
            outside of a method visibility 
        """ 
        if not workspace:
            workspace = mtd[self._ws_name]

        new_name = self._build_ws_name()
        old_name = workspace.name()
        if new_name != old_name:
           RenameWorkspace(InputWorkspace=old_name,OutputWorkspace=new_name)

           old_mon_name = old_name + '_monitors'
           new_mon_name = new_name + '_monitors'
           if old_mon_name in mtd:
              RenameWorkspace(InputWorkspace=old_mon_name,OutputWorkspace=new_mon_name)
        self._ws_name = new_name
#--------------------------------------------------------------------------------------------------------------------
    def get_file_ext(self):
        """ Method returns current file extension for file to load workspace from 
            e.g. .raw or .nxs extension
        """ 
        if self._run_ext:
            return self._run_ext
        else: # return IDF default
            return RunDescriptor._holder.data_file_ext
#--------------------------------------------------------------------------------------------------------------------
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
#--------------------------------------------------------------------------------------------------------------------
    @staticmethod
    def _check_claibration_source():
         """ if user have not specified calibration as input to the script, 
             try to retrieve calibration stored in file with run properties"""
         changed_prop = RunDescriptor._holder.getChangedProperties()
         if 'det_cal_file' in changed_prop:
              use_workspace_calibration = False
         else:
              use_workspace_calibration = True
         return use_workspace_calibration
#--------------------------------------------------------------------------------------------------------------------
    def get_workspace(self):
        """ Method returns workspace correspondent to current run number(s)
            and loads this workspace if it has not been loaded

            Returns Mantid pointer to the workspace, corresponding to this run number
        """ 
        if not self._ws_name:
           self._ws_name = self._build_ws_name()


        if self._ws_name in mtd:
            ws = mtd[self._ws_name]
            if ws.run().hasProperty("calibrated"):
                return ws # already calibrated
            else:
               prefer_ws_calibration = self._check_claibration_source()
               self.apply_calibration(ws,RunDescriptor._holder.det_cal_file,prefer_ws_calibration)
               return ws
        else:
           if self._run_number:
               prefer_ws_calibration = self._check_claibration_source()
               inst_name = RunDescriptor._holder.short_inst_name
               calibration = RunDescriptor._holder.det_cal_file
               if self._bind_to_sum and RunDescriptor._holder.sum_runs : # Sum runs
                   ws = RunDescriptor._PropMan.sum_runs.load_and_sum_runs(RunDescriptor._holder,inst_name,RunDescriptor._holder.load_monitors_with_workspace)
               else: # load current workspace
                   ws = self.load_run(inst_name, calibration,False, RunDescriptor._holder.load_monitors_with_workspace,prefer_ws_calibration)


               self.synchronize_ws(ws)
               self.apply_calibration(ws,calibration,prefer_ws_calibration)

               return ws
           else:
              return None
#--------------------------------------------------------------------------------------------------------------------
    def get_ws_clone(self,clone_name='ws_clone'):
        """ Get unbounded clone of eisting Run workspace """
        ws = self.get_workspace()
        CloneWorkspace(InputWorkspace=ws,OutputWorkspace=clone_name)
        mon_ws_name = self.get_ws_name() + '_monitors'
        if mon_ws_name in mtd:
            cl_mon_name = clone_name + '_monitors'
            CloneWorkspace(InputWorkspace=mon_ws_name,OutputWorkspace=cl_mon_name)

        return mtd[clone_name]
#--------------------------------------------------------------------------------------------------------------------
    def _set_ws_as_source(self,value):
        """ assign all parts of the run if input value is workspace """
        self._run_number = value.getRunNumber()
        ws_name = value.name()
        self._ws_suffix=''
        self._split_ws_name(ws_name)
        self.synchronize_ws(value)

#--------------------------------------------------------------------------------------------------------------------
    def chop_ws_part(self,origin,tof_range,rebin,chunk_num,n_chunks):
        """ chop part of the original workspace and sets it up as new original. 
            Return the old one """ 
        if not origin:
           origin = self.get_workspace()

        origin_name = origin.name()
        try:
           mon_ws = mtd[origin_name+'_monitors']
        except:
           mon_ws = None

        target_name = '#{0}/{1}#'.format(chunk_num,n_chunks)+origin_name
        if chunk_num == n_chunks:
           RenameWorkspace(InputWorkspace=origin_name,OutputWorkspace=target_name)
           if mon_ws:
              RenameWorkspace(InputWorkspace=mon_ws,OutputWorkspace=target_name+'_monitors')
           origin_name = target_name
           origin_invalidated=True
        else:
           if mon_ws:
              CloneWorkspace(InputWorkspace=mon_ws,OutputWorkspace=target_name+'_monitors')
           origin_invalidated=False

        if rebin: # debug and compatibility mode with old reduction
           Rebin(origin_name,OutputWorkspace=target_name,Params=[tof_range[0],tof_range[1],tof_range[2]],PreserveEvents=False)
        else:
           CropWorkspace(origin_name,OutputWorkspace=target_name,XMin=tof_range[0],XMax=tof_range[2])

        self._set_ws_as_source(mtd[target_name])
        if origin_invalidated:
            return self.get_workspace()
        else:
            return origin

#--------------------------------------------------------------------------------------------------------------------
    def get_monitors_ws(self,monitor_ID=None):
        """ get pointer to a workspace containing monitors. 

           Explores different ways of finding monitor workspace in Mantid and returns the python pointer to the
           workspace which contains monitors.
        """
        data_ws = self.get_workspace()

        monWS_name = self.get_ws_name() + '_monitors'
        if monWS_name in mtd:
            mon_ws = mtd[monWS_name]
            monitors_separate = True
        else:
            mon_ws = data_ws
            monitors_separate = False

        spec_to_mon = RunDescriptor._holder.spectra_to_monitors_list
        if monitors_separate and spec_to_mon :
            for specID in spec_to_mon:
                mon_ws = self.copy_spectrum2monitors(data_ws,mon_ws,specID)

        if monitor_ID:
           try:
               ws_index = mon_ws.getIndexFromSpectrumNumber(monitor_ID) 
           except: #
               mon_ws = None
        else: 
            mon_list = self._holder.get_used_monitors_list()
            for monID in mon_list:
                try:
                    ws_ind = mon_ws.getIndexFromSpectrumNumber(int(monID))
                except:
                   mon_ws = None
                   break
        return mon_ws
#--------------------------------------------------------------------------------------------------------------------
    def is_existing_ws(self):
        """ method verifies if property value relates to workspace, present in ADS """ 
        if self._ws_name:
            if self._ws_name in mtd:
                return True
            else:
                return False
        else:
           return False
#--------------------------------------------------------------------------------------------------------------------
    def get_ws_name(self):
        """ return workspace name. If ws name is not defined, build it first and set up as the target ws name
        """ 
        if self._ws_name:
            if self._ws_name in mtd:
                return self._ws_name
            else:
                raise RuntimeError('Getting workspace name {0} for undefined workspace. Run get_workspace first'.format(self._ws_name)) 

        self._ws_name = self._build_ws_name()
        return self._ws_name
#--------------------------------------------------------------------------------------------------------------------
    def file_hint(self,run_num_str=None,filePath=None,fileExt=None,**kwargs):
        """ procedure to provide run file guess name from run properties
         
            main purpose -- to support customized order of file extensions
        """ 
        if not run_num_str:
           run_num_str = str(self.run_number())


        inst_name = RunDescriptor._holder.short_inst_name
        if 'file_hint' in kwargs:
            hint = kwargs['file_hint']
            fname,old_ext = os.path.splitext(hint)
            if len(old_ext) == 0:
                old_ext = self.get_file_ext()
        else:
            if fileExt:
               old_ext = fileExt
            else:
               old_ext = self.get_file_ext()

            hint = inst_name + run_num_str + old_ext
            if not filePath:
                filePath = self._run_file_path
            if os.path.exists(filePath):
                hint = os.path.join(filePath,hint)
        if os.path.exists(hint):
            return hint,old_ext
        else:
            fp,hint = os.path.split(hint)
        return hint,old_ext
#--------------------------------------------------------------------------------------------------------------------

    def find_file(self,inst_name=None,run_num=None,filePath=None,fileExt=None,**kwargs):
        """Use Mantid to search for the given run. """

        if not inst_name:
            inst_name = RunDescriptor._holder.short_inst_name

        if run_num:
            run_num_str = str(run_num)
        else:
            run_num_str = str(self.run_number())
        #
        file_hint,old_ext = self.file_hint(run_num_str,filePath,fileExt,**kwargs)

        try:
            file = FileFinder.findRuns(file_hint)[0]
            fname,fex = os.path.splitext(file)
            self._run_ext = fex
            if old_ext != fex:
                message = '*** Cannot find run-file with extension {0}.\n'\
                          '    Found file {1} instead'.format(old_ext,file)
                RunDescriptor._logger(message,'notice')
            self._run_file_path = os.path.dirname(fname)
            return (True,file)
        except RuntimeError:
            message = 'Cannot find file matching hint {0} on current search paths ' \
                       'for instrument {1}'.format(file_hint,inst_name)
            if not 'be_quet' in kwargs:
                RunDescriptor._logger(message,'warning')
            return (False,message)
#--------------------------------------------------------------------------------------------------------------------

    def load_file(self,inst_name,ws_name,run_number=None,load_mon_with_workspace=False,filePath=None,fileExt=None,**kwargs):
        """ load run for the instrument name provided. If run_numner is None, look for the current run""" 
 
        ok,data_file = self.find_file(None,filePath,fileExt,**kwargs)
        if not ok:
           self._ws_name = None
           raise IOError(data_file)
                       
        if load_mon_with_workspace:
             mon_load_option = 'Include'
        else:
             mon_load_option = 'Separate'
        #
        try: # Hack: LoadEventNexus does not understand Separate at the moment and throws.
             # And event loader always loads monitors separately
             Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = mon_load_option)
        except ValueError:
             #mon_load_option =str(int(load_mon_with_workspace))
             Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = '1',MonitorsAsEvents='0')

        RunDescriptor._logger("Loaded {0}".format(data_file),'information')

        loaded_ws = mtd[ws_name]

        return loaded_ws
#--------------------------------------------------------------------------------------------------------------------

    def load_run(self,inst_name, calibration=None, force=False, mon_load_option=False,use_ws_calibration=True,\
                 filePath=None,fileExt=None,**kwargs):
        """Loads run into workspace with name provided.

           If force is true then the file is loaded regardless of whether this workspace already exists
        """
        # If a workspace with this name exists, then assume it is to be used in
        # place of a file
        if 'ws_name' in kwargs:
            ws_name = kwargs['ws_name']
            del kwargs['ws_name']
        else:
            try:
                ws_name = self.get_ws_name()
            except RuntimeError:
                self._ws_name = None
                ws_name = self.get_ws_name()
        #-----------------------------------
        if ws_name in mtd and not force:
            RunDescriptor._logger("{0} already loaded as workspace.".format(ws_name),'information')
        else:
            # If it doesn't exists as a workspace assume we have to try and
            # load a file
            loaded_ws = self.load_file(inst_name,ws_name,None,mon_load_option,filePath,fileExt,**kwargs)
        ######## Now we have the workspace
        self.apply_calibration(loaded_ws,calibration,use_ws_calibration)
        return loaded_ws
#--------------------------------------------------------------------------------------------------------------------
    def apply_calibration(self,loaded_ws,calibration=None,use_ws_calibration=True):
        """  If calibration is present, apply it to the workspace 
        
             use_ws_calibration -- if true, retrieve workspace property, which defines 
             calibration option (e.g. det_cal_file used a while ago) and try to use it
        """

        if not (calibration) or use_ws_calibration:
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
#--------------------------------------------------------------------------------------------------------------------
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
       x_param = mon_ws.readX(0)
       bins = [x_param[0],x_param[1] - x_param[0],x_param[-1]]
       ExtractSingleSpectrum(InputWorkspace=data_ws,OutputWorkspace='tmp_mon',WorkspaceIndex=ws_index)
       Rebin(InputWorkspace='tmp_mon',OutputWorkspace='tmp_mon',Params=bins,PreserveEvents='0')
       # should be vice versa but Conjoin invalidate ws pointers and hopefully
       # nothing could happen with workspace during conjoining
       #AddSampleLog(Workspace=monWS,LogName=done_log_name,LogText=str(ws_index),LogType='Number')
       mon_ws_name = mon_ws.getName()
       ConjoinWorkspaces(InputWorkspace1=mon_ws,InputWorkspace2='tmp_mon')
       mon_ws = mtd[mon_ws_name]

       if 'tmp_mon' in mtd:
           DeleteWorkspace(WorkspaceName='tmp_mon')
       return mon_ws
#--------------------------------------------------------------------------------------------------------------------
    def clear_monitors(self):
        """ method removes monitor workspace form analysis data service if it is there 
        
            (assuming it is not needed any more)
        """
        monWS_name = self._ws_name + '_monitors'
        if monWS_name in mtd:
            DeleteWorkspace(monWS_name)

#--------------------------------------------------------------------------------------------------------------------

    def _build_ws_name(self):

        instr_name = self._instr_name()

        sum_ext = RunDescriptor._PropMan.sum_runs.sum_ext()
        if self._run_number:
            ws_name = '{0}{1}{2}{3:0>#6d}{4}{5}'.format(self._prop_name,instr_name,self._ws_cname,self._run_number,sum_ext,self._ws_suffix)
        else:
            ws_name = '{0}{1}{2}{3}'.format(self._prop_name,self._ws_cname,sum_ext,self._ws_suffix)

        return ws_name 
#--------------------------------------------------------------------------------------------------------------------
    @staticmethod
    def rremove(thestr, trailing):
        thelen = len(trailing)
        if thestr[-thelen:] == trailing:
            return thestr[:-thelen]
        return thestr
    def _split_ws_name(self,ws_name):
        """ Method to split existing workspace name 
            into parts, in such a way that _build_name would restore the same name
        """
        # Remove suffix
        name = self.rremove(ws_name,self._ws_suffix)
        sumExt = RunDescriptor._PropMan.sum_runs.sum_ext()
        if len(sumExt) > 0:
            name = self.rremove(ws_name,sumExt)
        # remove _prop_name:
        name = name.replace(self._prop_name,'',1)

        try:
           part_ind = re.search('#(.+?)#', name).group(0)
           name     =name.replace(part_ind,'',1)
        except AttributeError:
           part_ind=''

        if self._run_number:
            instr_name = self._instr_name()
            name = name.replace(instr_name,'',1)
            self._ws_cname = part_ind+filter(lambda c: not c.isdigit(), name)

        else:
            self._ws_cname = part_ind+name
    #
    def _instr_name(self):
       if RunDescriptor._holder:
            instr_name = RunDescriptor._holder.short_inst_name
       else:
            instr_name = '_test_instrument'
       return instr_name 

    def _clear_old_ws(self,old_ws_name,new_name,clear_fext=False):
        """ helper method used in __set__. When new data (run or wod)  """
        if old_ws_name:
           if new_name != old_ws_name:
                if old_ws_name in mtd:
                   DeleteWorkspace(old_ws_name)
                old_mon_ws = old_ws_name + '_monitors'
                if old_mon_ws in mtd:
                    DeleteWorkspace(old_mon_ws)
                if clear_fext:
                   self._run_ext = None
                   self._run_file_path = ''

    def has_own_value(self):
        """ interface property used to verify if
            the class got its own values or been shadowed by 
            property, this one depends on 
            
        """ 
        return not(self._in_cash)
#-------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorDependent(RunDescriptor):
    """ Simple RunDescriptor class dependent on another RunDescriptor,
        providing the host descriptor if current descriptor value is not defined
        or usual descriptor functionality if somebody sets current descriptor up   
    """

    def __init__(self,host_run,ws_preffix,DocString=None):
        RunDescriptor.__init__(self,ws_preffix,DocString)
        self._host = host_run
        self._has_own_value = False

    def __get__(self,instance,owner=None):
       """ return dependent run number which is host run number if this one has not been set 
           or this run number if it was
       """ 
       if instance is None: # this class functions and the host functions
          return self

       if self._has_own_value: # this allows to switch between 
          return super(RunDescriptorDependent,self).__get__(instance,owner)
       else:
          return self._host.__get__(instance,owner)


    def __set__(self,instance,value):
        if value is None:
           self._has_own_value = False
           return
        self._has_own_value = True
        super(RunDescriptorDependent,self).__set__(instance,value)


    def has_own_value(self):
        """ interface property used to verify if
            the class got its own values or been shadowed by 
            property, this one depends on           
        """ 
        return self._has_own_value
    #--------------------------------------------------------------
    # TODO -- how to automate all these functions below?
    def run_number(self):
        if self._has_own_value:
           return super(RunDescriptorDependent,self).run_number()
        else:
           return self._host.run_number()
    #
    def is_monws_separate(self):
        if self._has_own_value:
           return super(RunDescriptorDependent,self).is_monws_separate()
        else:
           return self._host.is_monws_separate()

    def get_run_list(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_run_list()
        else:
            return self._host.get_run_list()

    def set_action_suffix(self,suffix=None):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).set_action_suffix(suffix)
        else:
            return self._host.set_action_suffix(suffix)

    def synchronize_ws(self,workspace=None):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).synchronize_ws(workspace)
        else:
            return self._host.synchronize_ws(workspace)

    def get_file_ext(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_file_ext()
        else:
            return self._host.get_file_ext()

    def set_file_ext(self,val):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).set_file_ex(val)
        else:
            return self._host.set_file_ex(val)

    def get_workspace(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_workspace()
        else:
            return self._host.get_workspace()

    def get_ws_clone(self,clone_name='ws_clone'):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_ws_clone()
        else:
            return self._host.get_ws_clone()

    def chop_ws_part(self,origin,tof_range,rebin,chunk_num,n_chunks):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).chop_ws_part(origin,tof_range,rebin,chunk_num,n_chunks)
        else:
            return self._host.chop_ws_part(origin,tof_range,rebin,chunk_num,n_chunks)

    def get_monitors_ws(self,monitor_ID=None):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_monitors_ws(monitor_ID)
        else:
            return self._host.get_monitors_ws(monitor_ID)

    def is_existing_ws(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).is_existing_ws()
        else:
            return self._host.is_existing_ws()

    def get_ws_name(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_ws_name()
        else:
            return self._host.get_ws_name()
       
    def file_hint(self,run_num_str=None,filePath=None,fileExt=None,**kwargs):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).file_hint(run_num_str,filePath,fileExt,**kwargs)
        else:
            return self._host.file_hint(run_num_str,filePath,fileExt,**kwargs)

    def find_file(self,inst_name=None,run_num=None,filePath=None,fileExt=None,**kwargs):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).find_file(inst_name,run_num,filePath,fileExt,**kwargs)
        else:
            return self._host.find_file(inst_name,run_num,filePath,fileExt,**kwargs)

    def load_file(self,inst_name,ws_name,run_number=None,load_mon_with_workspace=False,filePath=None,fileExt=None,**kwargs):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).load_file(inst_name,ws_name,run_number,load_mon_with_workspace,filePath,fileExt,**kwargs)
        else:
            return self._host.load_file(inst_name,ws_name,run_number,load_mon_with_workspace,filePath,fileExt,**kwargs)

    def load_run(self,inst_name, calibration=None, force=False, mon_load_option=False,use_ws_calibration=True,\
                 filePath=None,fileExt=None,**kwargs):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).load_run(inst_name,calibration, force, mon_load_option,use_ws_calibration,\
                 filePath,fileExt,**kwargs)
        else:
            return self._host.load_run(inst_name,calibration, force, mon_load_option,use_ws_calibration,\
                               filePath,fileExt,**kwargs)

    def apply_calibration(self,loaded_ws,calibration=None,use_ws_calibration=True):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).apply_calibration(loaded_ws,calibration,use_ws_calibration)
        else:
            return self._host.apply_calibration(loaded_ws,calibration,use_ws_calibration)

    def clear_monitors(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).clear_monitors()
        else:
            return self._host.clear_monitors()
    #--------------------------------------------------------------
