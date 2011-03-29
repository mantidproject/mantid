"""
    ISIS-specific implementation of the SANS Reducer. 
    
    WARNING: I'm still playing around with the ISIS reduction to try to 
    understand what's happening and how best to fit it in the Reducer design. 
     
"""
from reduction.instruments.sans.sans_reducer import SANSReducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
import isis_reduction_steps
from mantidsimple import *
import os
import copy

## Version number
__version__ = '0.0'

class ISISReducer(SANSReducer):
    """
        ISIS Reducer
        TODO: need documentation for all the data member
        TODO: need to see whether all those data members really belong here
    """    
    #the rebin parameters used by Q1D
    Q_REBIN = None
    QXY2 = None
    DQY = None

    BACKMON_START = None
    BACKMON_END = None

    # Component positions
    PHIMIN=-90.0
    PHIMAX=90.0
    PHIMIRROR=True
    
    ## Path for user settings files
    _user_file_path = '.'
    
    def __init__(self):
        SANSReducer.__init__(self)

        self._init_steps()
        self.wksp_name = None
        self.full_trans_wav = True
        self._monitor_set = False

        self._prepare_raw = []
        self._fork_ws = []
        self._conv_Q = []
        self._can = []
        self._tidy = []

    def _to_steps(self):
        """
            Defines the steps that are run and their order
        """
#        self._reduction_steps.append(self.data_loader)
#        self._reduction_steps.append(self.user_settings)
#        self._prepare_raw.append(self.place_det_sam)
        self._prepare_raw.append(self.geometry)
        #---- creates a new workspace leaving the raw data behind 
        self._fork_ws.append(self.out_name)
        #---- the can special reducer uses the steps starting with the next one
        self._conv_Q.append(self.flood_file)
        self._conv_Q.append(self.crop_detector)
        self._conv_Q.append(self.samp_trans_load)
        self._conv_Q.append(self.mask)
        self._conv_Q.append(self.to_wavelen)
        self._conv_Q.append(self.norm_mon)
        self._conv_Q.append(self.transmission_calculator)
        self._conv_Q.append(self._corr_and_scale)
        self._conv_Q.append(self._geo_corr)
        self._conv_Q.append(self.to_Q)
        #---- the can special reducer ends on the previous step
        self._can.append(self.background_subtracter)
        
        self._tidy.append(self._zero_error_flags)
        self._tidy.append(self._rem_zeros)
        
        self._reduction_steps = self._prepare_raw + self._fork_ws + self._conv_Q + self._can+ self._tidy

    def _init_steps(self):
        """
            Initialises the steps that are not initialised by (ISIS)CommandInterface.
        """
        #_to_steps() defines the order the steps are run in, any steps not in that list wont be run  
        
        self.data_loader =     None
        self.user_settings =   None
        self.place_det_sam =   isis_reduction_steps.MoveComponents()
        self.geometry =        sans_reduction_steps.GetSampleGeom()
        self.out_name =       isis_reduction_steps.GetOutputName()
        self.flood_file =      isis_reduction_steps.CorrectToFileISIS(
            '', 'SpectrumNumber','Divide', self.out_name.name_holder)
        self.crop_detector =   isis_reduction_steps.CropDetBank(
            self.out_name.name_holder)
        self.samp_trans_load = None
        self.can_trans_load =  None
        self.mask =self._mask= isis_reduction_steps.Mask_ISIS()
        self.to_wavelen =      sans_reduction_steps.UnitsConvert('Wavelength')
        self.norm_mon =        isis_reduction_steps.NormalizeToMonitor()
        self.transmission_calculator =\
                               isis_reduction_steps.TransmissionCalc(loader=None)
        self._corr_and_scale = isis_reduction_steps.ISISCorrections()
        self.to_Q =            isis_reduction_steps.ConvertToQ()
        self.background_subtracter = None
        self._geo_corr =       sans_reduction_steps.SampleGeomCor(self.geometry)
        self._zero_error_flags=isis_reduction_steps.ReplaceErrors()
        self._rem_zeros =      sans_reduction_steps.StripEndZeros()

    def pre_process(self): 
        """
            Reduction steps that are meant to be executed only once per set
            of data files. After this is executed, all files will go through
            the list of reduction steps.
        """
        self._to_steps()

    def _reduce(self):
        """
            Execute the list of all reduction steps
        """
        # Check that an instrument was specified
        if self.instrument is None:
            raise RuntimeError, "Reducer: trying to run a reduction without an instrument specified"

        # Go through the list of steps that are common to all data files
        self.pre_process()

        #self._data_files[final_workspace] = self._data_files[file_ws]
        #del self._data_files[file_ws]
        #----> can_setup.setReducedWorkspace(tmp_can)
        
        #Correct(sample_setup, wav_start, wav_end, use_def_trans, finding_centre)
        self.run_steps(start_ind=0, stop_ind=len(self._reduction_steps))

        #any clean up, possibly removing workspaces 
        self.post_process()
        self.clean = False
        
        return self.wksp_name
    
    def reduce_another(self, to_reduce, new_wksp=None):
        """
            Apply the sample corrections to another workspace, used by CanSubtraction
            @param to_reduce: the workspace that will be corrected
            @param new_wksp: the name of the workspace that will store the result (default the name of the input workspace)
        """
        if not new_wksp:
            new_wksp = to_reduce

        # Can correction
        new_reducer = copy.deepcopy(self)

        #give the name of the new workspace to the first algorithm that was run
        new_reducer.flood_file.out_container[0] = new_wksp
        #the line below is required if the step above is optional
        new_reducer.crop_detector.out_container[0] = new_wksp
        
        if new_reducer.transmission_calculator:
            new_reducer.transmission_calculator.set_loader(new_reducer.can_trans_load)

        norm_step_ind = new_reducer.step_num(new_reducer.norm_mon)
        new_reducer._reduction_steps[norm_step_ind] = \
            isis_reduction_steps.NormalizeToMonitor(raw_ws=to_reduce)
           
        #set the workspace that we've been setting up as the one to be processed 
        new_reducer.set_process_single_workspace(to_reduce)
        self.run_conv_Q(new_reducer)

    def run_from_raw(self):
        """
            Executes all the steps after moving the components
        """
        self.run_steps(
                       start_ind=self.step_num(self._fork_ws[0]),
                       stop_ind=len(self._reduction_steps))

        #any clean up, possibly removing workspaces 
        self.post_process()
        self.clean = False
        
        return self.wksp_name

    def run_conv_Q(self, reducer=None):
        """
            Executes all the commands required to correct a can workspace
        """
        if not reducer:
            reducer = self
            
        steps = reducer._conv_Q
        #the reducer is completely setup, run it
        reducer.run_steps(
                          start_ind=reducer.step_num(steps[0]),
                          stop_ind=reducer.step_num(steps[len(steps)-1]))

    def run_steps(self, start_ind = None, stop_ind = None):
        """
            Run part of the chain, starting at the first specified step
            and ending at the last. If start or finish are set to None
            they will default to the first and last steps in the chain
            respectively. No pre- or post-processing is done. Assumes
            there are no duplicated steps
            @param start_ind the index number of the first step to run
            @param end_ind the index of the last step that will be run
        """
        if start_ind is None:
            start_ind = 0

        if stop_ind is None:
            stop_ind = len(self._reduction_steps)

        for file_ws in self._data_files:
            self.wksp_name = self._data_files.values()[0]
            for item in self._reduction_steps[start_ind:stop_ind+1]:
                if not item is None:
                    item.execute(self, self.wksp_name)


                #TODO: change the following
                finding_centre = False
        
                    
                if finding_centre:
                    self.final_workspace = file_ws.split('_')[0] + '_quadrants'
                 
                # Crop Workspace to remove leading and trailing zeroes
                #TODO: deal with this once we have the final workspace name sorted out
                if finding_centre:
                    quadrants = {1:'Left', 2:'Right', 3:'Up',4:'Down'}
                    for key, value in quadrants.iteritems():
                        old_name = self.final_workspace + '_' + str(key)
                        RenameWorkspace(old_name, value)

        self.clean = False
        return self.wksp_name


    def post_process(self):
        # Store the mask file within the final workspace so that it is saved to the CanSAS file
        if self.user_settings is None:
            user_file = 'None'
        else:
            user_file = self.user_settings.filename
        AddSampleLog(self.wksp_name, "UserFile", user_file)

    def set_user_path(self, path):
        """
            Set the path for user files
            @param path: user file path
        """
        if os.path.isdir(path):
            self._user_file_path = path
        else:
            raise RuntimeError, "ISISReducer.set_user_path: provided path is not a directory (%s)" % path

    def get_user_path(self):
        return self._user_file_path
    
    user_file_path = property(get_user_path, set_user_path, None, None)

    def set_run_number(self, workspace):
        """
            The run number is a number followed by a . and then
            the extension of the run file to load
            @param workspace: optional name of the workspace for this data,
                default will be the name of the file 
        """
        self._data_files.clear()
        self._data_files[workspace] = workspace

    def get_sample(self):
        """
            Returns the name of the raw workspace that was
            loaded to run
        """ 
        if len(self._data_files) > 0:
            return self._data_files.values()[0]
        else:
            return None
    
    def set_background(self, can_run=None, reload = True, period = -1):
        """
            Sets the can data to be subtracted from sample data files
            @param data_file: Name of the can run file
        """
        if can_run is None:
            self.background_subtracter = None
        else:
            self.background_subtracter = isis_reduction_steps.CanSubtraction(can_run, reload=reload, period=period)

    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log"):
        self.transmission_calculator.set_trans_fit(lambda_min, lambda_max, fit_method, override=True)
        self.transmission_calculator.enabled = True
        
    def set_trans_sample(self, sample, direct, reload=True, period_t = -1, period_d = -1):
        if not issubclass(self.samp_trans_load.__class__, sans_reduction_steps.BaseTransmission):
            self.samp_trans_load = isis_reduction_steps.LoadTransmissions(reload=reload)
        self.samp_trans_load.set_trans(sample, period_t)
        self.samp_trans_load.set_direc(direct, period_d)
        self.transmission_calculator.set_loader(self.samp_trans_load)

    def set_trans_can(self, can, direct, reload = True, period_t = -1, period_d = -1):
        if not issubclass(self.can_trans_load.__class__, sans_reduction_steps.BaseTransmission):
            self.can_trans_load = isis_reduction_steps.LoadTransmissions(is_can=True, reload=reload)
        self.can_trans_load.set_trans(can, period_t)
        self.can_trans_load.set_direc(direct, period_d)

    def set_monitor_spectrum(self, specNum, interp=False, override=True):
        if override:
            self._monitor_set=True
        
        if not self._monitor_set or override:
            self.instrument.set_incident_mon(specNum)
            self.instrument.set_interpolating_norm(interp)
                        
    def suggest_monitor_spectrum(self, specNum, interp=False):
        if not self._monitor_set:
            self.instrument.suggest_incident_mntr(specNum)
            self.instrument.suggest_interpolating_norm(interp)
                    
    def set_trans_spectrum(self, specNum, interp=False):
        self.instrument.incid_mon_4_trans_calc = int(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if interp :
            self.instrument.use_interpol_trans_calc = True                    

    def get_trans_lambdamin(self):
        """
            Gets the value of the lowest wavelength that is to
            be used in the transmission calculation
        """
        return self.transmission_calculator.get_lambdamin(self.instrument)

    def get_trans_lambdamax(self):
        """
            Gets the value of the highest wavelength that is to
            be used in the transmission calculation
        """
        return self.transmission_calculator.get_lambdamax(self.instrument)

    def set_process_single_workspace(self, wk_name):
        """
            Clears the list of data files and inserts one entry
            to the workspace whose name is given
        """
        self._data_files = {'dummy' : wk_name}

    def step_num(self, step):
        """
            Returns the index number of a step in the
            list of steps that have _so_ _far_ been
            added to the chain
        """
        return self._reduction_steps.index(step)

    def get_instrument(self):
        """
            Convenience function used by the inst property to make
            calls shorter
        """
        return self.instrument
 
    #quicker to write than .instrument 
    inst = property(get_instrument, None, None, None)
 
    def Q_string(self):
        return '    Q range: ' + self.Q_REBIN +'\n    QXY range: ' + self.QXY2+'-'+self.DQXY

    def ViewCurrentMask(self):
        self._mask.view(self.instrument)

    def reference(self):
        return self

    CENT_FIND_RMIN = None
    CENT_FIND_RMAX = None
