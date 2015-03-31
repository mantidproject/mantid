#pylint: disable=invalid-name
""" File contains Descriptors used describe run for direct inelastic reduction """

from mantid.simpleapi import *
from Direct.PropertiesDescriptors import *
import re

class RunList(object):
    """Helper class to maintain list of runs used in RunDescriptor for summing
       or subsequent processing range of files.

       Supports basic operations with this list
    """
    def __init__(self,theRumDescr,run_list,file_names=None,fext=None):
        """ """
        self._theRun = theRumDescr
        self._last_ind2sum = -1
        self._file_path = None
        self._fext = None
        self.set_list2add(run_list,file_names,fext)
        self._partial_sum_ws_name = None
   #
    def set_list2add(self,runs_to_add,fnames=None,fext=None):
        """Set run numbers to add together with possible file guess-es """
        if not isinstance(runs_to_add,list):
            raise KeyError('Can only set list of run numbers to add')
        runs = []
        for item in runs_to_add:
            runs.append(int(item))
        self._run_numbers = runs
        self._set_fnames(fnames,fext)
#--------------------------------------------------------------------------------------------------
    def set_cashed_sum_ws(self,ws,new_ws_name=None):
        """Store the name of a workspace in the class
           as reference
        """
        if new_ws_name:
            old_name = ws.name()
            if old_name != new_ws_name:
                old_mon_name = old_name + '_monitors'
                RenameWorkspace(ws,OutputWorkspace=new_ws_name)
            if old_mon_name in mtd:
                RenameWorkspace(old_mon_name,OutputWorkspace=new_ws_name + '_monitors')
        else:
            new_ws_name = ws.name()
        self._partial_sum_ws_name = new_ws_name
    #
    def get_cashed_sum_ws(self):
        """Return python pointer to cached sum workspace
        """
        if not self._partial_sum_ws_name:
            return None
        if self._partial_sum_ws_name in mtd:
            return mtd[self._partial_sum_ws_name]
        else:
            return None
    #
    def get_cashed_sum_clone(self):
        """ """
        origin = self.get_cashed_sum_ws()
        if not origin:
            return None
        origin_name = origin.name()
        mon_name = origin_name + '_monitors'
        if mon_name in mtd:
            CloneWorkspace(InputWorkspace=mon_name,OutputWorkspace=origin_name + '_clone_monitors')
        ws = CloneWorkspace(InputWorkspace=origin_name,OutputWorkspace=origin_name + '_clone')
        return ws
    #
    def del_cashed_sum(self):
        """ """
        if not self._partial_sum_ws_name:
            return
        if self._partial_sum_ws_name in mtd:
            DeleteWorkspace(self._partial_sum_ws_name)
        mon_ws = self._partial_sum_ws_name + '_monitors'
        if mon_ws in mtd:
            DeleteWorkspace(mon_ws)
#--------------------------------------------------------------------------------------------------
    #
    def _set_fnames(self,fnames,fext):
        """Sets filenames lists and file extension lists
            of length correspondent to run number length

           if length of the list provided differs from the length
           of the run list, expands fnames list and fext list
           to the whole runnumber list using last for fext and
           first for fnames members of the
        """
        if fnames:
            if isinstance(fnames,list):
                self._file_path = fnames
            else:
                self._file_path = [fnames]

        if not self._file_path:
            self._file_path = [''] * len(self._run_numbers)
        else:
            if len(self._file_path) != len(self._run_numbers):
                self._file_path = [self._file_path[0]] * len(self._run_numbers)

        if fext:
            if isinstance(fext,list):
                self._fext = fext
            else:
                self._fext = [fext]

        if not self._fext:
            self._fext = [None] * len(self._run_numbers)
        else:
            if len(self._fext) != len(self._run_numbers):
                base_fext = self._fext[-1]
                self._fext = [base_fext] * len(self._run_numbers)
    #
    def get_fext(self,index=0):
        """Get file extension for file with run number
           Should be used on defined Run_list only(which should be always true)
        """
        fext_given =self._fext[index]
        if fext_given is None:
            return self._theRun._holder.data_file_ext
        else:
            return fext_given
    #
    def get_file_guess(self,inst_name,run_num,index=None):
        """Return the name of run file for run number provided

          Note: internal file extension overwrites
          default_fext if internal is not empty
        """
        if index is None:
            index = self._run_numbers.index(run_num)
        path_guess = self._file_path[index]
        fext = self.get_fext(index)

        guess = build_run_file_name(run_num,inst_name,path_guess,fext)
        return (guess,index)
    #
    def get_run_file_list(self,inst_name):
        """Return list of files, used corresponding to runs"""
        run_files = []
        for ind,run in enumerate(self._run_numbers):
            fname,index = self.get_file_guess(inst_name,run,ind)
            run_files.append(fname)
        return run_files
    #
    def get_all_run_list(self):
        return self._run_numbers
    #
    def add_or_replace_run(self,run_number,fpath='',fext=None):
        """Add run number to list of existing runs

         Let's prohibit adding the same run numbers using this method.
         Equivalent run numbers can still be added using list assignment

         file path and file extension are added/modified if present
         regardless of run being added or replaced
        """
        if not run_number in self._run_numbers:
            self._run_numbers.append(run_number)
            if not fpath:
                fpath = self._file_path[-1]
            self._file_path.append(fpath)

            self._fext.append(fext)

            self._last_ind2sum = len(self._run_numbers) - 1
            return self._last_ind2sum
        else:
            ext_ind = self._run_numbers.index(run_number)
            if len(fpath) > 0:
                self._file_path[ext_ind] = fpath
            if fext: #not to keep existing extension if new one is provided
                self._fext[ext_ind] = fext
            self._last_ind2sum = ext_ind
            return ext_ind
    #
    def check_runs_equal(self,run_list,fpath=None,fext=None):
        """Returns true if all run numbers in existing list are
           in the comparison list and vice versa.

           if lists numbers coincide,
           sets new file_path and fext list if such are provided
        """
        if len(run_list) != len(self._run_numbers):
            return False

        for run in run_list:
            if not run in self._run_numbers:
                return False
        self._set_fnames(fpath,fext)
        return True
    #
    def get_current_run_info(self,sum_runs,ind=None):
        """Return last run info for file to sum"""
        if ind:
            if not(ind > -1 and ind < len(self._run_numbers)):
                raise RuntimeError("Index {0} is outside of the run list of {1} runs".format(ind,len(self._run_numbers)))
        else:
            ind = self.get_last_ind2sum(sum_runs)
        return self._run_numbers[ind],self._file_path[ind],self.get_fext(ind),ind
    #
    def set_last_ind2sum(self,run_number):
        """Check and set last number, contributing to summation
           if this number is out of summation range, clear the summation
        """
        run_number = int(run_number)
        if run_number in self._run_numbers:
            self._last_ind2sum = self._run_numbers.index(run_number)
        else:
            self._last_ind2sum = -1
    #
    def get_run_list2sum(self,num_to_sum=None):
        """Get run numbers of the files to be summed together
           from the list of defined run numbers
        """
        n_runs = len(self._run_numbers)
        if num_to_sum:
            if num_to_sum <= 0:
                num_to_sum = 1
            if num_to_sum > n_runs:
                num_to_sum = n_runs
        else:
            num_to_sum = n_runs

        if self._last_ind2sum >= 0 and self._last_ind2sum < num_to_sum:
            num_to_sum = self._last_ind2sum + 1

        return self._run_numbers[:num_to_sum]
    #
    def get_last_ind2sum(self,sum_runs):
        """Get last run number contributing to sum"""

        if self._last_ind2sum >= 0 and self._last_ind2sum < len(self._run_numbers):
            ind = self._last_ind2sum
        else:
            if sum_runs:
                ind = len(self._run_numbers) - 1
            else:
                ind = 0
        return ind
    #
    def sum_ext(self,sum_runs):
        if sum_runs:
            last = self.get_last_ind2sum(sum_runs)
            sum_ext = "SumOf{0}".format(len(self._run_numbers[:last + 1]))
        else:
            sum_ext = ''
        return sum_ext
    #
    def find_run_files(self,inst_name,run_list=None):
        """Find run files correspondent to the run list provided
          and set path to these files as new internal parameters
          for the files in list

          Return the list of the runs, which files were
          not found and found

          Run list have to coincide or be part of self._run_numbers
          No special check for correctness is performed, so may fail
          miserably
        """

        if not run_list:
            run_list = self._run_numbers
        not_found = []
        found = []
        for run in run_list:
            file_hint,index = self.get_file_guess(inst_name,run)
            try:
                file = FileFinder.findRuns(file_hint)[0]
                fpath,fname = os.path.split(file)
                fname,fex = os.path.splitext(fname)
                self._fext[index] = fex
                self._file_path[index] = fpath
                #self._last_ind2sum = index
                found.append(run)
            except RuntimeError:
                not_found.append(run)
        return not_found,found
#--------------------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------------------
class RunDescriptor(PropDescriptor):
    """Descriptor to work with a run or list of runs specified
       either as run number (run file) or as
       this run loaded in memory as a workspace

    """
    # the host class referencing contained all instantiated descriptors.
    # Descriptors methods rely on it to work (e.g.  to extract file loader
    # preferences)
    # so it has to be set up manually by PropertyManager __init__ method
    _holder = None
    _logger = None
    _sum_log_name = 'SumRuns'
#--------------------------------------------------------------------------------------------------------------------
    def __init__(self,prop_name,DocString=None):
        """ """
        self._prop_name = prop_name
        if not DocString is None:
            self.__doc__ = DocString

        self._ws_name = None
        # pointer to workspace used to mask this workspace obtained at diag for
        # this ws
        self._mask_ws_name = None

        self._clear_all()

    def __len__(self):
        """overloaded len function, which
           return length of the run-files list
           to work with
        """
        if not self._run_number:
            return 0
        if self._run_list:
            return len(self._run_list._run_numbers)
        else:
            return 1
#--------------------------------------------------------------------------------------------------------------------
    def _clear_all(self):
        """clear all internal properties, workspaces and caches,
           associated with this run
        """
        # Run number
        self._run_number = None
        # Extension of the file to load data from
        #
        self._run_file_path = ''
        self._fext = None

        if self._ws_name:
            mon_ws = self._ws_name + '_monitors'
            # Workspace name which corresponds to the run
            if self._ws_name in mtd:
                DeleteWorkspace(self._ws_name)
            if mon_ws in mtd:
                DeleteWorkspace(mon_ws)

        self._ws_name = None # none if not loaded
        # String used to identify the workspace related to this property
        #  w.r.t.  other workspaces
        self._ws_cname = ''
        self._ws_suffix = ''
        # property contains run lists
        self._run_list = None
        #
        self._in_cash = False
        # clear masking workspace if any available
        if self._mask_ws_name:
            if self._mask_ws_name in mtd:
                DeleteWorkspace(self._mask_ws_name)
            self._mask_ws_name = None

#--------------------------------------------------------------------------------------------------------------------
    def __get__(self,instance,owner):
        """Return current run number or workspace if it is loaded"""
        if instance is None:
            return self

        if self._ws_name and self._ws_name in mtd:
            return mtd[self._ws_name]
        else:
            return self._run_number
#--------------------------------------------------------------------------------------------------------------------
    def __set__(self,instance,value):
        """Set up Run number and define workspace name from any source """
        #
        if value == None: # clear current run number
            self._clear_all()
            return
        if isinstance(value, api.Workspace):
            if  self._ws_name:
                if self._ws_name != value.name():
                    self._clear_all()
                    self._set_ws_as_source(value)
                else:
                    return # do nothing
                # it is just reassigning the same workspace to itself
            else: # first assignment of workspace to property
                self._set_ws_as_source(value)
            return

        if isinstance(value,str): # it may be run number as string or it may be a workspace name
            if value in mtd: # workspace name
                ws = mtd[value]
                self.__set__(instance,ws)
                return
            else:  # split string into run indexes and auxiliary file parameters
                file_path,run_num,fext = prop_helpers.parse_run_file_name(value)

                if isinstance(run_num,list):
                    self._set_run_list(instance,run_num,file_path,fext)
                else:
                    self._set_single_run(instance,run_num,file_path,fext)
        elif isinstance(value,list):
            self._set_run_list(instance,value,"",None)
        else:
            self._set_single_run(instance,value,"",None)

#--------------------------------------------------------------------------------------------------------------------
    def get_fext(self):
        """Return actual file extension for given run regardless of it
           has been set or not
        """
        if self._fext is None:
            return self._holder.data_file_ext
        else:
            return self._fext
#--------------------------------------------------------------------------------------------------------------------

    def _set_single_run(self,instance,run_number,file_path='',fext=None):
        """ """
        self._run_number = int(run_number)
        # build workspace name for current run number
        new_ws_name = self._build_ws_name()

        if self._run_list and instance.sum_runs:
            ind = self._run_list.add_or_replace_run(self._run_number,file_path,fext)
            self._run_file_path = self._run_list._file_path[ind]
            self._fext = self._run_list._fext[ind]
            self._ws_name = new_ws_name
        else:
            if self._ws_name != new_ws_name:
                self._clear_all()
                # clear all would invalidate run number and workspace number
                self._run_number = int(run_number)
                self._ws_name = self._build_ws_name()
                self._run_file_path = file_path
                self._fext = fext
            else: # nothing to do, there is workspace, which corresponds to this run number
                # and it may be already loaded (may be not).  Just nullify run list
                                 # in case of previous workspace name came from a list.
                self._run_list = None
                if not self._ws_name in mtd:
                    # Change existing file path and file extension if alternatives are provided
                    if len(file_path)>0:
                        self._run_file_path = file_path
                    if not fext is None: # Change only if real new extension is provided
                        self._fext = fext



#--------------------------------------------------------------------------------------------------------------------
    def _set_run_list(self,instance,run_list,file_path=None,fext=None):

        if self._run_list and self._run_list.check_runs_equal(run_list,file_path,fext):
            return
        else:
            self._clear_all()
            self._run_list = RunList(self,run_list,file_path,fext)
            run_num,file_path,main_fext,ind = self._run_list.get_current_run_info(instance.sum_runs)
            self._run_list.set_last_ind2sum(ind)
            self._run_number = run_num
            self._run_file_path = file_path
            if fext is None:
                self._fext = None
            else:
                self._fext = main_fext
            self._ws_name = self._build_ws_name()

    def run_number(self):
        """Return run number regardless of workspace is loaded or not"""
        if self._ws_name and self._ws_name in mtd:
            ws = mtd[self._ws_name]
            return ws.getRunNumber()
        else:
            return self._run_number
#--------------------------------------------------------------------------------------------------------------------
# Masking
#--------------------------------------------------------------------------------------------------------------------
    def get_masking(self,noutputs=None):
        """Return masking workspace specific to this particular workspace
           together with number of masked spectra if requested.

           noutputs is provided as argument, as funcreturn does not propagate
           through inheritance and overloaded functions
        """
        if not noutputs:
            try:
                noutputs,r = funcreturns.lhs_info('both')
            except:
                noutputs=0

        if self._mask_ws_name:
            mask_ws = mtd[self._mask_ws_name]
            #TODO: need normal exposure of getNumberMasked() method of masks workspace
            if noutputs>1:
                __tmp_masks,spectra = ExtractMask(self._mask_ws_name)
                num_masked = len(spectra)
                DeleteWorkspace(__tmp_masks)
                return (mask_ws,num_masked)
            else:
                return mask_ws
        else:
            if noutputs>1:
                return (None,0)
            else:
                return None
#--------------------------------------------------------------------------------------------------------------------
    def add_masked_ws(self,masked_ws):
        """Extract masking from the workspace provided and store masks
           to use with this run workspace
        """
        if self._mask_ws_name:
            mask_ws = mtd[self._mask_ws_name]
            add_mask_name = self._prop_name + '_tmp_masking'
        else:
            add_mask_name = self._prop_name + 'CurrentMasking'

        masks,spectra = ExtractMask(InputWorkspace=masked_ws,OutputWorkspace=add_mask_name)
        if self._mask_ws_name:
            mask_ws +=masks
            DeleteWorkspace(add_mask_name)
        else:
            self._mask_ws_name = add_mask_name
        #
#--------------------------------------------------------------------------------------------------------------------
    def is_monws_separate(self):
        """Is monitor workspace is separated from data workspace or not"""
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
        """Returns list of the files, assigned to current property """
        current_run = self.run_number()
        if self._run_list:
            runs = self._run_list.get_all_run_list()
            if current_run in runs:
                return runs
            else:
                return [current_run]
        else:
            return [current_run]
#--------------------------------------------------------------------------------------------------------------------
    def get_run_file_list(self):
        """Returns list of the files, assigned to current property """

        inst = RunDescriptor._holder.short_inst_name
        fext = self.get_fext()
        run_num = self.run_number()
        current_run = build_run_file_name(run_num,inst,self._run_file_path,fext)
        if self._run_list:
            runs = self._run_list.get_all_run_list()
            if run_num in runs:
                runf = self._run_list.get_run_file_list(inst)
                return runf
            else:
                return [current_run]
        else:
            return [current_run]

#--------------------------------------------------------------------------------------------------------------------
    @staticmethod
    def get_sum_run_list(ws):
        """Retrieve list of contributed run numbers from the sum workspace log"""

        summed_runs = []
        if RunDescriptor._sum_log_name in ws.getRun():
            summed_str = ws.getRun().getLogData(RunDescriptor._sum_log_name).value
            run_nums = summed_str.split(',')
            for run_str in run_nums:
                summed_runs.append(int(run_str))
        else:
            raise RuntimeError("Presumably sum workspace {0} does not have sum log attached to it".format(ws.name()))
        return summed_runs
#--------------------------------------------------------------------------------------------------------------------
    def get_runs_to_sum(self,existing_sum_ws=None,num_files=None):
        """Return list of runs, expected to be summed together
            excluding the runs, already summed and added to cached sum workspace
        """

        if not RunDescriptor._holder.sum_runs:
            return ([],None,0)
        if not self._run_list:
            return ([],None,0)
        #
        summed_runs = []
        if not existing_sum_ws:
            existing_sum_ws = self._run_list.get_cashed_sum_ws()
        if existing_sum_ws:
            summed_runs = RunDescriptor.get_sum_run_list(existing_sum_ws)
        n_existing_sums = len(summed_runs)

        runs2_sum = self._run_list.get_run_list2sum(num_files)
        for run in summed_runs:
            if run in runs2_sum:
                del runs2_sum[runs2_sum.index(run)]
        return (runs2_sum,existing_sum_ws,n_existing_sums)
#--------------------------------------------------------------------------------------------------------------------
    def find_run_files(self,run_list=None):
        """Find run files correspondent to the run list provided
          and set path to these files as new internal parameters
          for the files in the list

          Returns True and empty list or False and
          the list of the runs, which files were not found
          or not belong to the existing run list.
        """

        if not self._run_list:
            if not run_list:
                return (True,[],[])
            else:
                return (False,run_list,[])

        if run_list:
            existing = self._run_list.get_all_run_list()
            non_existing = []
            for run in run_list:
                if not run in existing:
                    raise RuntimeError('run {0} is not in the existing run list'.format(run))
        not_found=[]
        found = []
        inst = RunDescriptor._holder.short_instr_name
        not_found,found = self._run_list.find_run_files(inst,run_list)
        if len(not_found) == 0:
            Ok = True
        else:
            Ok = False
        return (Ok,not_found,found)
#--------------------------------------------------------------------------------------------------------------------
    def set_action_suffix(self,suffix=None):
        """Method to set part of the workspace name, which indicate some action performed over this workspace
           e.g.: default suffix of a loaded workspace is 'RAW' but we can set it to SPE to show that conversion to
           energy will be performed for this workspace.

           method returns the name of the workspace is will have with this suffix.
           Algorithms would later  work on the initial workspace and modify it in-place or to produce workspace
           with new name (depending if one wants to keep initial workspace)

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
        """Synchronize workspace name (after workspace may have changed due to algorithm)
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
    @staticmethod
    def _check_calibration_source():
        """If user have not specified calibration as input to the script,
            try to retrieve calibration stored in file with run properties"""
        changed_prop = RunDescriptor._holder.getChangedProperties()
        if 'det_cal_file' in changed_prop:
            use_workspace_calibration = False
        else:
            use_workspace_calibration = True
        return use_workspace_calibration
#--------------------------------------------------------------------------------------------------------------------
    def get_workspace(self):
        """Method returns workspace correspondent to current run number(s)
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
                prefer_ws_calibration = self._check_calibration_source()
                self.apply_calibration(ws,RunDescriptor._holder.det_cal_file,prefer_ws_calibration)
                return ws
        else:
            if self._run_number:
                prefer_ws_calibration = self._check_calibration_source()
                inst_name = RunDescriptor._holder.short_inst_name
                calibration = RunDescriptor._holder.det_cal_file
                if self._run_list and RunDescriptor._holder.sum_runs : # Sum runs
                    ws = self._load_and_sum_runs(inst_name,RunDescriptor._holder.load_monitors_with_workspace)
                else: # load current workspace
                    ws = self.load_run(inst_name, calibration,False, RunDescriptor._holder.load_monitors_with_workspace,prefer_ws_calibration)


                self.synchronize_ws(ws)
                self.apply_calibration(ws,calibration,prefer_ws_calibration)

                return ws
            else:
                return None
#--------------------------------------------------------------------------------------------------------------------
    def get_ws_clone(self,clone_name='ws_clone'):
        """Get unbounded clone of existing Run workspace"""
        ws = self.get_workspace()
        CloneWorkspace(InputWorkspace=ws,OutputWorkspace=clone_name)
        mon_ws_name = ws.name() + '_monitors'
        if mon_ws_name in mtd:
            cl_mon_name = clone_name + '_monitors'
            CloneWorkspace(InputWorkspace=mon_ws_name,OutputWorkspace=cl_mon_name)

        return mtd[clone_name]
#--------------------------------------------------------------------------------------------------------------------
    def _set_ws_as_source(self,value):
        """Assign all parts of the run if input value is workspace"""
        self._run_number = value.getRunNumber()
        ws_name = value.name()
        self._ws_suffix = ''
        self._split_ws_name(ws_name)
        self.synchronize_ws(value)

#--------------------------------------------------------------------------------------------------------------------
    def chop_ws_part(self,origin,tof_range,rebin,chunk_num,n_chunks):
        """Chop part of the original workspace and sets it up to this run as new original
           Return the pointer to workspace being chopped
        """
        if not origin:
            origin = self.get_workspace()

        origin_name = origin.name()
        try:
            mon_ws = mtd[origin_name + '_monitors']
        except:
            mon_ws = None

        target_name = '#{0}/{1}#'.format(chunk_num,n_chunks) + origin_name
        if chunk_num == n_chunks:
            RenameWorkspace(InputWorkspace=origin_name,OutputWorkspace=target_name)
            if mon_ws:
                RenameWorkspace(InputWorkspace=mon_ws,OutputWorkspace=target_name + '_monitors')
            origin_name = target_name
            origin_invalidated = True
        else:
            if mon_ws:
                CloneWorkspace(InputWorkspace=mon_ws,OutputWorkspace=target_name + '_monitors')
            origin_invalidated = False

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
    def get_monitors_ws(self,monitors_ID=None,otherWS=None):
        """Get pointer to a workspace containing monitors.

           Explores different ways of finding monitor workspace in Mantid and returns the python pointer to the
           workspace which contains monitors.
        """
        if otherWS:
            data_ws  = otherWS
        else:
            data_ws = self.get_workspace()
        if not data_ws:
            return None

        monWS_name = data_ws.name() + '_monitors'
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

        if monitors_ID:
            if isinstance(monitors_ID,list):
                mon_list = monitors_ID
            else:
                mon_list = [monitors_ID]
        else:
            mon_list = self._holder.get_used_monitors_list()
        #
        for monID in mon_list:
            try:
                ws_ind = mon_ws.getIndexFromSpectrumNumber(int(monID))
            except:
                try:
                    monws_name = mon_ws.name()
                except: 
                    monws_name = 'None'
                RunDescriptor._logger('*** Monitor workspace {0} does not have monitor with ID {1}. Monitor workspace set to None'.\
                                          format(monws_name,monID),'warning')
                mon_ws = None
                break
        return mon_ws
#--------------------------------------------------------------------------------------------------------------------
    def is_existing_ws(self):
        """Method verifies if property value relates to workspace, present in ADS"""
        if self._ws_name:
            if self._ws_name in mtd:
                return True
            else:
                return False
        else:
            return False
#--------------------------------------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------------------------------------
    def set_file_ext(self,val):
        """Set non-default file extension """
        if isinstance(val,str):
            if val[0] != '.':
                value = '.' + val
            else:
                value = val
            self._fext = value
        else:
            raise AttributeError('Source file extension can be only a string')

    def file_hint(self,run_num_str=None,filePath=None,fileExt=None,**kwargs):
        """Procedure to provide run file guess name from run properties

           main purpose -- to support customized order of file extensions
        """
        if not run_num_str:
            run_num_str = str(self.run_number())
        inst_name = RunDescriptor._holder.short_inst_name

        if 'file_hint' in kwargs:
            hint = kwargs['file_hint']
            fname,old_ext = os.path.splitext(hint)
            if len(old_ext) == 0:
                old_ext = self.get_fext()
        else:
            old_ext = self.get_fext()
            if fileExt is None:
                fileExt = old_ext
            if filePath is None:
                filePath = self._run_file_path
            fname = build_run_file_name(run_num_str,inst_name,filePath,fileExt)

        if os.path.exists(fname):
            return fname,old_ext
        else:
            fp,hint = os.path.split(fname)
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
            self._fext = fex
            if old_ext != fex:
                message = '*** Cannot find run-file with extension {0}.\n'\
                          '    Found file {1} instead'.format(old_ext,file)
                RunDescriptor._logger(message,'notice')
            self._run_file_path = os.path.dirname(fname)
            return (True,file)
        except RuntimeError:
            message = '*** Cannot find file matching hint {0} on Mantid search paths '.\
                       format(file_hint)
            if not 'be_quet' in kwargs:
                RunDescriptor._logger(message,'warning')
            return (False,message)
#--------------------------------------------------------------------------------------------------------------------

    def load_file(self,inst_name,ws_name,run_number=None,load_mon_with_workspace=False,filePath=None,fileExt=None,**kwargs):
        """Load run for the instrument name provided. If run_numner is None, look for the current run"""

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
            ws_name = self._build_ws_name()
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
        """If calibration is present, apply it to the workspace

           use_ws_calibration -- if true, retrieve workspace property, which defines
           calibration option (e.g. det_cal_file used a while ago) and try to use it
        """

        if not calibration or use_ws_calibration:
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
            try:
                ws_index = data_ws.getIndexFromSpectrumNumber(spectraID)
            except: 
                raise RuntimeError('*** Error: Can not retrieve spectra with ID {0} from source workspace: {1}'.\
                                    format(spectraID,data_ws.name()))

        #
        x_param = mon_ws.readX(0)
        homo_binning,dx_min=RunDescriptor._is_binning_homogeneous(x_param)
        bins = [x_param[0],dx_min,x_param[-1]]
        ExtractSingleSpectrum(InputWorkspace=data_ws,OutputWorkspace='tmp_mon',WorkspaceIndex=ws_index)
        Rebin(InputWorkspace='tmp_mon',OutputWorkspace='tmp_mon',Params=bins,PreserveEvents='0')
        mon_ws_name = mon_ws.getName()
        if not homo_binning:
            Rebin(InputWorkspace=mon_ws_name,OutputWorkspace=mon_ws_name,Params=bins,PreserveEvents='0')
        ConjoinWorkspaces(InputWorkspace1=mon_ws_name,InputWorkspace2='tmp_mon')
        mon_ws = mtd[mon_ws_name]

        if 'tmp_mon' in mtd:
            DeleteWorkspace(WorkspaceName='tmp_mon')
        return mon_ws
    #
    @staticmethod
    def _is_binning_homogeneous(x_param):
        """Verify if binning in monitor workspace is homogeneous"""
        dx=x_param[1:]-x_param[0:-1]
        dx_min=min(dx)
        dx_max=max(dx)
        if dx_max-dx_min>1.e-9:
            return False,dx_min
        else:
            return True,dx_min

#--------------------------------------------------------------------------------------------------------------------
    def clear_monitors(self):
        """ method removes monitor workspace form analysis data service if it is there
            (assuming it is not needed any more)
        """
        monWS_name = self._ws_name + '_monitors'
        if monWS_name in mtd:
            DeleteWorkspace(monWS_name)
#--------------------------------------------------------------------------------------------------------------------
    def clear_resulting_ws(self):
        """Remove workspace from memory as if it has not been processed
           and clear all operations indicators except cashes and run lists.

           Attempt to get workspace for a file based run should in this case
           load workspace again
        """
        ws_name = self._ws_name
        mon_name = ws_name + '_monitors'

        self._ws_name = ''
        self._ws_cname = ''
        self._ws_suffix = ''
        if ws_name in mtd:
            ws = mtd[ws_name]
            self._run_number = ws.getRunNumber()
            DeleteWorkspace(ws_name)
        if mon_name in mtd:
            DeleteWorkspace(mon_name)
        if self._run_list:
            ind = self._run_list.add_or_replace_run(self._run_number)
            self._run_file_path = self._run_list._file_path[ind]
            self._fext = self._run_list.get_fext(ind)
#--------------------------------------------------------------------------------------------------------------------

    def _build_ws_name(self,sum_runs=None):
        instr_name = self._instr_name()
        if self._run_list:
            if not sum_runs:
                sum_runs = RunDescriptor._holder.sum_runs
            sum_ext = self._run_list.sum_ext(sum_runs)
        else:
            sum_ext = ''

        if self._run_number:
            ws_name = '{0}{1}{2}{3:0>#6d}{4}{5}'.format(self._prop_name,instr_name,self._ws_cname,self._run_number,\
                                                        sum_ext,self._ws_suffix)
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
        """Method to split existing workspace name
           into parts, in such a way that _build_name would restore the same name
        """
        # Remove suffix
        name = self.rremove(ws_name,self._ws_suffix)
        if self._run_list:
            summed = RunDescriptor._holder.sum_runs
            sumExt = self._run_list.sum_ext(summed)
        else:
            sumExt = ''
        if len(sumExt) > 0:
            name = self.rremove(ws_name,sumExt)
        # remove _prop_name:
        name = name.replace(self._prop_name,'',1)

        try:
            part_ind = re.search('#(.+?)#', name).group(0)
            name = name.replace(part_ind,'',1)
        except AttributeError:
            part_ind = ''

        if self._run_number:
            instr_name = self._instr_name()
            name = name.replace(instr_name,'',1)
            self._ws_cname = part_ind + filter(lambda c: not c.isdigit(), name)
        else:
            self._ws_cname = part_ind + name
    #
    def _instr_name(self):
        if RunDescriptor._holder:
            instr_name = RunDescriptor._holder.short_inst_name
        else:
            instr_name = '_test_instrument'
        return instr_name


    def has_own_value(self):
        """Interface property used to verify if
           the class got its own values or been shadowed by
           property, this one depends on
        """
        return not self._in_cash

    def notify_sum_runs_changed(self,old_value,new_value):
        """Take actions on changes to sum_runs option
        """
        if self._run_list:
            if old_value != new_value:
                rl = self._run_list
                self._clear_all()
                rl.set_last_ind2sum(-1) # this will reset index to default
                self._run_list = rl
                run_num,file_path,main_fext,ind = self._run_list.get_current_run_info(new_value)
                self._run_list.set_last_ind2sum(ind)
                self._run_number = run_num
                self._run_file_path = file_path
                self._fext = main_fext
                self._ws_name = self._build_ws_name(new_value)
            if new_value is False:
                self._run_list.del_cashed_sum()

    def _load_and_sum_runs(self,inst_name,monitors_with_ws):
        """Load multiple runs and sum them together

           monitors_with_ws -- if true, load monitors with workspace
        """

        RunDescriptor._logger("*** Summing multiple runs            ****")

        runs_to_sum,sum_ws,n_already_summed = self.get_runs_to_sum()
        num_to_sum = len(runs_to_sum)

        if sum_ws:
            RunDescriptor._logger("*** Use cached sum of {0} workspaces and adding {1} remaining".\
                          format(n_already_summed,num_to_sum))
            sum_ws_name = sum_ws.name()
            sum_mon_name = sum_ws_name + '_monitors'
            AddedRunNumbers = sum_ws.getRun().getLogData(RunDescriptor._sum_log_name).value
            load_start = 0
        else:
            RunDescriptor._logger("*** Loading #{0}/{1}, run N: {2} ".\
                   format(1,num_to_sum,runs_to_sum[0]))

            f_guess,index = self._run_list.get_file_guess(inst_name,runs_to_sum[0])
            ws = self.load_file(inst_name,'Sum_ws',False,monitors_with_ws,
                                False,file_hint=f_guess)

            sum_ws_name = ws.name()
            sum_mon_name = sum_ws_name + '_monitors'
            #AddedRunNumbers = [ws.getRunNumber()]
            AddedRunNumbers = str(ws.getRunNumber())
            load_start = 1
        #end

        for ind,run_num in enumerate(runs_to_sum[load_start:num_to_sum]):

            RunDescriptor._logger("*** Adding  #{0}/{1}, run N: {2} ".\
                          format(ind + 1 + load_start,num_to_sum,run_num))

            term_name = '{0}_ADDITIVE_#{1}/{2}'.format(inst_name,ind + 1 + load_start,num_to_sum)#
            f_guess,index = self._run_list.get_file_guess(inst_name,run_num)

            wsp = self.load_file(inst_name,term_name,False,
                                monitors_with_ws,False,file_hint=f_guess)

            wsp_name = wsp.name()
            wsp_mon_name = wsp_name + '_monitors'
            Plus(LHSWorkspace=sum_ws_name,RHSWorkspace=wsp_name,
                OutputWorkspace=sum_ws_name,ClearRHSWorkspace=True)
            #  AddedRunNumbers.append(run_num)
            AddedRunNumbers+=',' + str(run_num)
            if not monitors_with_ws:
                Plus(LHSWorkspace=sum_mon_name,RHSWorkspace=wsp_mon_name,
                     OutputWorkspace=sum_mon_name,ClearRHSWorkspace=True)
            if wsp_name in mtd:
                DeleteWorkspace(wsp_name)
            if wsp_mon_name in mtd:
                DeleteWorkspace(wsp_mon_name)
        #end for
        RunDescriptor._logger("*** Summing multiple runs  completed ****")

        #AddSampleLog(Workspace=sum_ws_name,LogName =
        #RunDescriptor._sum_log_name,
        #             LogText=AddedRunNumbers,LogType='Number Series')
        AddSampleLog(Workspace=sum_ws_name,LogName = RunDescriptor._sum_log_name,
                    LogText=AddedRunNumbers,LogType='String')

        if RunDescriptor._holder.cashe_sum_ws:
            # store workspace in cash for further usage
            self._run_list.set_cashed_sum_ws(mtd[sum_ws_name],self._prop_name + 'Sum_ws')
            ws = self._run_list.get_cashed_sum_clone()
        else:
            ws = mtd[sum_ws_name]
        return ws

#-------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorDependent(RunDescriptor):
    """Simple RunDescriptor class dependent on another RunDescriptor,
       providing the host descriptor if current descriptor value is not defined
       or usual descriptor functionality if somebody sets current descriptor up
    """

    def __init__(self,host_run,ws_preffix,DocString=None):
        RunDescriptor.__init__(self,ws_preffix,DocString)
        self._host = host_run
        self._has_own_value = False

    def __get__(self,instance,owner=None):
        """Return dependent run number which is host run number if this one has not been set
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
        """Interface property used to verify if
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

    def get_run_files_list(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_run_files_list()
        else:
            return self._host.get_run_files_list()

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

    def get_fext(self):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_fext()
        else:
            return self._host.get_fext()

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
    def get_masking(self,noutputs=None):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).get_masking(noutputs)
        else:
            return self._host.get_masking(noutputs)
    def add_masked_ws(self,masked_ws):
        if self._has_own_value:
            return super(RunDescriptorDependent,self).add_masked_ws(masked_ws)
        else:
            return self._host.add_masked_ws(masked_ws)
#--------------------------------------------------------------------------------------------------------------------
#--------------------------------------------------------------------------------------------------------------------
def build_run_file_name(run_num,inst,file_path='',fext=''):
    """Build the full name of a runfile from all possible components"""
    if fext is None:
        fext = ''
    fname = '{0}{1}{2}'.format(inst,run_num,fext)
    if not file_path is None:
        if os.path.exists(file_path):
            fname = os.path.join(file_path,fname)
    return fname


