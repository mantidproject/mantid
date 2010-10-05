"""
    Implementation of reduction steps for ISIS SANS instruments
    
    Most of this code is a copy-paste from SANSReduction.py, organized to be used with
    ReductionStep objects. The guts needs refactoring.
"""
from Reducer import ReductionStep
import SANSReductionSteps
from mantidsimple import *
import SANSUtility
import SANSInsts
import os
import math

#TODO: remove when center finding is working 
DEL__FINDING_CENTRE_ = False

def _issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    mantid.sendLogMessage('::SANS::Warning: ' + msg)


class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    def __init__(self, data_file=None, spec_min=None, spec_max=None, period=1):
        #TODO: data_file = None only makes sense when AppendDataFile is used... (AssignSample?)
        super(LoadRun, self).__init__()
        self._data_file = data_file
        self._spec_min = spec_min
        self._spec_max = spec_max
        self._period = period
        
    def execute(self, reducer, workspace):
        # If we don't have a data file, look up the workspace handle
        if self._data_file is None:
            if workspace in reducer._data_files:
                #self._data_file = reducer._data_files[workspace]
                self._data_file = reducer._full_file_path(reducer._data_files[workspace])
            else:
                raise RuntimeError, "ISISReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        
        if os.path.splitext(self._data_file)[1].lower().startswith('.n'):
            alg = LoadNexus(self._data_file, workspace, SpectrumMin=self._spec_min, SpectrumMax=self._spec_max)
        else:
            alg = LoadRaw(self._data_file, workspace, SpectrumMin=self._spec_min, SpectrumMax=self._spec_max)
            LoadSampleDetailsFromRaw(workspace, self._data_file)
    
        pWorksp = mtd[workspace]
    
        if pWorksp.isGroup() :
            #get the number of periods in a group using the fact that each period has a different name
            nNames = len(pWorksp.getNames())
            numPeriods = nNames - 1
            workspace = _leaveSinglePeriod(pWorksp, self._period)
            pWorksp = mtd[workspace]
        else :
            #if the work space isn't a group there is only one period
            numPeriods = 1
            
        if (self._period > numPeriods) or (self._period < 1):
            raise ValueError('_loadRawData: Period number ' + str(self._period) + ' doesn\'t exist in workspace ' + pWorksp.getName())
        
        # Return the file path actually used to load the data
        fullpath = alg.getPropertyValue("Filename")

        return [ os.path.dirname(fullpath), workspace, numPeriods]        

    # Helper function
    def _assignHelper(self, run_string, is_trans, reload = True, period = -1, reducer=None):
        if run_string == '' or run_string.startswith('.'):
            return SANSUtility.WorkspaceDetails('', -1),True,'','', -1

        wkspname, run_no, logname, data_file = extract_workspace_name(run_string, is_trans, 
                                                            prefix=reducer.instrument.name(), 
                                                            run_number_width=reducer.instrument.run_number_width)

        if run_no == '':
            return SANSUtility.WorkspaceDetails('', -1),True,'','', -1

        if reload == False and mtd.workspaceExists(wkspname):
            return WorkspaceDetails(wkspname, shortrun_no),False,'','', -1

        filename = os.path.join(reducer._data_path, data_file)
        # Workaround so that the FileProperty does the correct searching of data paths if this file doesn't exist
        if not os.path.exists(filename):
            filename = data_file
        if period <= 0:
            period = 1
        if is_trans:
            try:
                if reducer.instrument.name() == 'SANS2D' and int(run_no) < 568:
                    dimension = SANSUtility.GetInstrumentDetails(reducer.instrument)[0]
                    specmin = dimension*dimension*2
                    specmax = specmin + 4
                else:
                    specmin = None
                    specmax = 8

                loader = LoadRun(filename, spec_min=specmin, spec_max=specmax, period=period)
                [filepath, wkspname, nPeriods] = loader.execute(reducer, wkspname)
            except RuntimeError, err:
                mantid.sendLogMessage("::SANS::Warning: "+str(err))
                return SANSUtility.WorkspaceDetails('', -1),True,'','', -1
        else:
            try:
                loader = LoadRun(filename, spec_min=None, spec_max=None, period=period)
                [filepath, wkspname, nPeriods] = loader.execute(reducer, wkspname)
            except RuntimeError, details:
                mantid.sendLogMessage("::SANS::Warning: "+str(details))
                return SANSUtility.WorkspaceDetails('', -1),True,'','', -1

        inWS = SANSUtility.WorkspaceDetails(wkspname, run_no)
        
        return inWS,True, reducer.instrument.name() + logname, filepath, nPeriods

    def _leaveSinglePeriod(self, groupW, period):
        #get the name of the individual workspace in the group
        oldName = groupW.getName()+'_'+str(period)
        #move this workspace out of the group (this doesn't delete it)
        groupW.remove(oldName)
    
        discriptors = groupW.getName().split('_')       #information about the run (run number, if it's 1D or 2D, etc) is listed in the workspace name between '_'s
        for i in range(0, len(discriptors) ):           #insert the period name after the run number
            if i == 0 :                                 #the run number is the first part of the name
                newName = discriptors[0]+'p'+str(period)#so add the period number here
            else :
                newName += '_'+discriptors[i]
    
        RenameWorkspace(oldName, newName)
    
        #remove the rest of the group
        mtd.deleteWorkspace(groupW.getName())
        return newName
    
    def _clearPrevious(self, inWS, others = []):
        if inWS != None:
            if type(inWS) == SANSUtility.WorkspaceDetails:
                inWS = inWS.getName()
            if mtd.workspaceExists(inWS) and (not inWS in others):
                mtd.deleteWorkspace(inWS)
                
class LoadTransmissions(SANSReductionSteps.BaseTransmission, LoadRun):
    """
        Transmission calculation for ISIS SANS instruments
    """
    # Map input values to Mantid options
    TRANS_FIT_OPTIONS = {
        'YLOG' : 'Log',
        'STRAIGHT' : 'Linear',
        'CLEAR' : 'Off',
        # Add Mantid ones as well
        'LOG' : 'Log',
        'LINEAR' : 'Linear',
        'LIN' : 'Linear',
        'OFF' : 'Off'
    }  
    _lambda_min = None
    _lambda_max = None
    _fit_method = 'Log'
    
    # Transmission sample parameters
    _direct_sample = None
    _trans_sample = None
    _sample_reload = True
    _sample_period = -1
    
    # Transmission can parameters
    _direct_can = None
    _trans_can = None
    _can_reload = True
    _can_period = -1
    
    TRANS_SAMPLE = '' 
    DIRECT_SAMPLE = ''
    TRANS_SAMPLE_N_PERIODS = -1
    DIRECT_SAMPLE_N_PERIODS = -1
    TRANS_CAN = ''
    DIRECT_CAN = ''
    TRANS_CAN_N_PERIODS = -1
    DIRECT_CAN_N_PERIODS = -1
    
    def __init__(self):
        """
        """
        super(LoadTransmissions, self).__init__()
    
    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log"):
        self._lambda_min = lambda_min
        self._lambda_max = lambda_max
        fit_method = fit_method.upper()
        if fit_method in self.TRANS_FIT_OPTIONS.keys():
            self._fit_method = self.TRANS_FIT_OPTIONS[fit_method]
        else:
            self._fit_method = 'Log'      
            mantid.sendLogMessage("ISISReductionStep.Transmission: Invalid fit mode passed to TransFit, using default LOG method")
    
    def set_trans_sample(self, sample, direct, reload=True, period=-1):            
        self._trans_sample = sample
        self._direct_sample = direct
        self._sample_reload = reload
        self._sample_period = period
        
    def set_trans_can(self, can, direct, reload = True, period = -1):
        self._trans_can = can
        self._direct_can = direct
        self._can_reload = reload
        self._can_period = period
    
    def execute(self, reducer, workspace):
        """
            Calls the CalculateTransmission algorithm
        """ 
        if self._lambda_max == None:
            self._lambda_max = reducer.instrument.TRANS_WAV1_FULL
        if self._lambda_min == None:
            self._lambda_min = reducer.instrument.TRANS_WAV2_FULL
        
        # Load transmission sample
        self._clearPrevious(self.TRANS_SAMPLE)
        self._clearPrevious(self.DIRECT_SAMPLE)
        
        if self._trans_sample not in [None, '']:
            trans_ws, dummy1, dummy2, dummy3, self.TRANS_SAMPLE_N_PERIODS = \
                self._assignHelper(self._trans_sample, True, self._sample_reload, self._sample_period, reducer)
            self.TRANS_SAMPLE = trans_ws.getName()
        
        if self._direct_sample not in [None, '']:
            direct_sample_ws, dummy1, dummy2, dummy3, self.DIRECT_SAMPLE_N_PERIODS = \
                self._assignHelper(self._direct_sample, True, self._sample_reload, self._sample_period, reducer)
            self.DIRECT_SAMPLE = direct_sample_ws.getName()
        
        # Load transmission can
        self._clearPrevious(self.TRANS_CAN)
        self._clearPrevious(self.DIRECT_CAN)
    
        if self._trans_can not in [None, '']:
            can_ws, dummy1, dummy2, dummy3, self.TRANS_CAN_N_PERIODS = \
                self._assignHelper(self._trans_can, True, self._can_reload, self._can_period, reducer)
            self.TRANS_CAN = can_ws.getName()
            
        if self._direct_can in [None, '']:
            self.DIRECT_CAN, self.DIRECT_CAN_N_PERIODS = self.DIRECT_SAMPLE, self.DIRECT_SAMPLE_N_PERIODS
        else:
            direct_can_ws, dummy1, dummy2, dummy3, self.DIRECT_CAN_N_PERIODS = \
                self._assignHelper(self._direct_can, True, self._can_reload, self._can_period, reducer)
            self.DIRECT_CAN = direct_can_ws.getName()
 
        #TODO: Call CalculateTransmission

class CanSubtraction(LoadRun):
    """
        Subtract the can after correcting it.
        Note that in the original SANSReduction.py, the can run was loaded immediately after
        the AssignCan() command was called. Since the loading needs information from the instrument, 
        we load only before doing the subtraction.
    """

    SCATTER_CAN = None
    _CAN_SETUP = None
    _CAN_RUN = None
    _CAN_N_PERIODS = -1
    #TODO: we don't need a dictionary here
    PERIOD_NOS = { "SCATTER_SAMPLE":1, "SCATTER_CAN":1 }

    def __init__(self, can_run, reload = True, period = -1):
        """
            @param lambda_min: MinWavelength parameter for CalculateTransmission
            @param lambda_max: MaxWavelength parameter for CalculateTransmission
            @param fit_method: FitMethod parameter for CalculateTransmission (Linear or Log)
        """
        super(CanSubtraction, self).__init__()
        self._can_run = can_run
        self._can_run_reload = reload
        self._can_run_period = period

    def _assign_can(self, reducer, can_run, reload = True, period = -1):
        #TODO: get rid of any reference to the instrument object as much as possible
        # Definitely get rid of the if-statements checking the instrument name.
        if not issubclass(reducer.instrument.__class__, SANSInsts.ISISInstrument):
            raise RuntimeError, "Transmission.assign_can expects an argument of class ISISInstrument"
        
        
        # Code from AssignCan
        self._clearPrevious(self.SCATTER_CAN)
        
        self._CAN_N_PERIODS = -1
        
        if( can_run.startswith('.') or can_run == '' or can_run == None):
            self.SCATTER_CAN.reset()
            self._CAN_RUN = ''
            self._CAN_SETUP = None
            return '', '()'
    
        self._CAN_RUN = can_run
        self.SCATTER_CAN ,reset, logname,filepath, self._CAN_N_PERIODS = \
            self._assignHelper(can_run, False, reload, period, reducer)
        if self.SCATTER_CAN.getName() == '':
            mantid.sendLogMessage('::SANS::Warning: Unable to load sans can run, cannot continue.')
            return '','()'
        if reset == True:
            self._CAN_SETUP  = None
            

        try:
            logvalues = reducer.instrument.load_detector_logs(logname,filepath)
            if logvalues == None:
                _issueWarning("Can logs could not be loaded, using sample values.")
                return self.SCATTER_CAN.getName(), "()"
        except AttributeError:
            if not reducer.instrument.name() == 'LOQ' : raise
    
        self.PERIOD_NOS["SCATTER_CAN"] = period
    
        if (reducer.instrument.name() == 'LOQ'):
            return self.SCATTER_CAN.getName(), ""
        
        smp_values = []
        front_det = reducer.instrument.getDetector('front')
        smp_values.append(reducer.instrument.FRONT_DET_Z + front_det.z_corr)
        smp_values.append(reducer.instrument.FRONT_DET_X + front_det.x_corr)
        smp_values.append(reducer.instrument.FRONT_DET_ROT + front_det.rot_corr)
        rear_det = reducer.instrument.getDetector('rear')
        smp_values.append(reducer.instrument.REAR_DET_Z + rear_det.z_corr)
        smp_values.append(reducer.instrument.REAR_DET_X + rear_det.x_corr)
    
        # Check against sample values and warn if they are not the same but still continue reduction
        if len(logvalues) == 0:
            return  self.SCATTER_CAN.getName(), logvalues
        
        can_values = []
        can_values.append(float(logvalues['Front_Det_Z']) + front_det.z_corr)
        can_values.append(float(logvalues['Front_Det_X']) + front_det.x_corr)
        can_values.append(float(logvalues['Front_Det_Rot']) + front_det.rot_corr)
        can_values.append(float(logvalues['Rear_Det_Z']) + rear_det.z_corr)
        can_values.append(float(logvalues['Rear_Det_X']) + rear_det.x_corr)
    
    
        det_names = ['Front_Det_Z', 'Front_Det_X','Front_Det_Rot', 'Rear_Det_Z', 'Rear_Det_X']
        for i in range(0, 5):
            if math.fabs(smp_values[i] - can_values[i]) > 5e-04:
                mantid.sendLogMessage("::SANS::Warning: values differ between sample and can runs. Sample = " + str(smp_values[i]) + \
                                  ' , Can = ' + str(can_values[i]))
                reducer.instrument.append_marked(det_names[i])
        # End of AssignCan code
        
        return self.SCATTER_CAN.getName(), logvalues

    def execute(self, reducer, workspace):
        """
        """ 
        if self._can_run is not None:
            self._assign_can(reducer, self._can_run, reload = self._can_run_reload, period = self._can_run_period)
        
        # Apply same corrections as for data then subtract from data
        # Start of WaveRangeReduction code
        # _initReduction code
        # _init_run()
        beamcoords = reducer._beam_finder.get_beam_center()
        mantid.sendLogMessage('::SANS:: Initializing can workspace to [' + str(beamcoords[0]) + ',' + str(beamcoords[1]) + ']' )

        final_ws = "can_temp_workspace"

        # Put the components in the correct positions
        currentDet = reducer.instrument.cur_detector().name() 
        maskpt_rmin, maskpt_rmax = reducer.instrument.set_component_positions(self.SCATTER_CAN.getName(), beamcoords[0], beamcoords[1])
        
        # Create a run details object
        TRANS_CAN = ''
        DIRECT_CAN = ''
        if reducer._transmission_calculator is not None:
            TRANS_CAN = reducer._transmission_calculator.TRANS_CAN
        if reducer._transmission_calculator is not None:
            DIRECT_CAN = reducer._transmission_calculator.DIRECT_CAN
            
        can_setup = SANSUtility.RunDetails(self.SCATTER_CAN, final_ws, TRANS_CAN, DIRECT_CAN, maskpt_rmin, maskpt_rmax, 'can')
        # End _init_run            
        # End of _initReduction  
        
        finding_centre = False
        
        
        tmp_smp = workspace+"_sam_tmp"
        RenameWorkspace(workspace, tmp_smp)
        # Run correction function
        # was  Correct(SCATTER_CAN, can_setup[0], can_setup[1], wav_start, wav_end, can_setup[2], can_setup[3], finding_centre)
        tmp_can = workspace+"_can_tmp"
        can_setup.setReducedWorkspace(tmp_can)
        # Can correction
        #TODO: deal with this once we have the final workspace name (setReducedWorkspace) sorted
        if False:
            Correct(can_setup, wav_start, wav_end, use_def_trans, finding_centre)
            Minus(tmp_smp, tmp_can, workspace)
    
            # Due to rounding errors, small shifts in detector encoders and poor stats in highest Q bins need "minus" the
            # workspaces before removing nan & trailing zeros thus, beware,  _sc,  _sam_tmp and _can_tmp may NOT have same Q bins
            if finding_centre == False:
                ReplaceSpecialValues(InputWorkspace = tmp_smp,OutputWorkspace = tmp_smp, NaNValue="0", InfinityValue="0")
                ReplaceSpecialValues(InputWorkspace = tmp_can,OutputWorkspace = tmp_can, NaNValue="0", InfinityValue="0")
                if reducer.CORRECTION_TYPE == '1D':
                    rem_zeros = SANSReductionSteps.StripEndZeros()
                    rem_zeros.execute(reducer, tmp_smp)
                    rem_zeros.execute(reducer, tmp_can)
            else:
                mantid.deleteWorkspace(tmp_smp)
                mantid.deleteWorkspace(tmp_can)
                mantid.deleteWorkspace(workspace)        
        # End of WaveRangeReduction  
    
class Mask_ISIS(SANSReductionSteps.Mask):
    """
        Provides ISIS specific mask functionality (e.g. parsing
        MASK commands from user files), inherits from Mask
    """
    def __init__(self, timemask='', timemask_r='', timemask_f='', 
                 specmask='', specmask_r='', specmask_f=''):
        SANSReductionSteps.Mask.__init__(self)
        self._timemask=timemask 
        self._timemask_r=timemask_r
        self._timemask_f=timemask_f
        self._specmask=specmask
        self._specmask_r=specmask_r
        self._specmask_f=specmask_f
        self._lim_phi_xml = ''
        
        ########################## Masking  ################################################
        # Mask the corners and beam stop if radius parameters are given

        self._maskpt_rmin = [0.0,0.0]
        self._min_radius = None
        self._max_radius = None

    def set_radi(self, min, max):
        self._min_radius = float(min)/1000.
        self._max_radius = float(max)/1000.

    def parse_instruction(self, details):
        """
            Parse an instruction line from an ISIS mask file
        """
        details = details.lstrip()
        details_compare = details.upper()
        if not details_compare.startswith('MASK'):
            _issueWarning('Ignoring malformed mask line ' + details)
            return
        
        parts = details_compare.split('/')
        # A spectrum mask or mask range applied to both detectors
        if len(parts) == 1:
            spectra = details[4:].lstrip()
            if len(spectra.split()) == 1:
                self._specmask += ',' + spectra
        elif len(parts) == 2:
            type = parts[1]
            detname = type.split()
            if type == 'CLEAR':
                self._specmask = ''
                self._specmask_r = ''
                self._specmask_f = ''
            elif type.startswith('T'):
                if type.startswith('TIME'):
                    bin_range = type[4:].lstrip()
                else:
                    bin_range = type[1:].lstrip()
                self._timemask += ';' + bin_range
            elif len(detname) == 2:
                det_type = detname[0]
                #TODO: warning: this means that Detector needs to be called before Mask
                if self.instrument.isDetectorName(det_type) :
                    spectra = detname[1]
                    if self.instrument.isHighAngleDetector(type) :
                        self._specmask_f += ',' + spectra
                    else:
                        self._specmask_r += ',' + spectra
                else:
                    _issueWarning('Detector \'' + det_type + '\' not found in currently selected instrument ' + self.instrument.name() + '. Skipping line.')
            else:
                _issueWarning('Unrecognized masking option "' + details + '"')
        elif len(parts) == 3:
            type = parts[1]
            if type == 'CLEAR':
                self._timemask = ''
                self._timemask_r = ''
                self._timemask_f = ''
            elif (type == 'TIME' or type == 'T'):
                parts = parts[2].split()
                if len(parts) == 3:
                    detname = parts[0].rstrip()
                    bin_range = parts[1].rstrip() + ' ' + parts[2].lstrip() 
                    if self.instrument.detectorExists(detname) :
                        if self.instrument.isHighAngleDetector(detname) :
                            self._timemask_f += ';' + bin_range
                        else:
                            self._timemask_r += ';' + bin_range
                    else:
                        _issueWarning('Detector \'' + det_type + '\' not found in currently selected instrument ' + self.instrument.name() + '. Skipping line.')
                else:
                    _issueWarning('Unrecognized masking option "' + details + '"')
        else:
            pass

    def _ConvertToSpecList(self, maskstring, detector):
        '''
            Convert a mask string to a spectra list
            6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)
        '''
        #Compile spectra ID list
        if maskstring == '':
            return ''
        masklist = maskstring.split(',')
        
        orientation = reducer.instrument.get_orientation
        firstspec = detector.first_spec_num
        dimension = detector.n_columns
        speclist = ''
        for x in masklist:
            x = x.lower()
            if '+' in x:
                bigPieces = x.split('+')
                if '>' in bigPieces[0]:
                    pieces = bigPieces[0].split('>')
                    low = int(pieces[0].lstrip('hv'))
                    upp = int(pieces[1].lstrip('hv'))
                else:
                    low = int(bigPieces[0].lstrip('hv'))
                    upp = low
                if '>' in bigPieces[1]:
                    pieces = bigPieces[1].split('>')
                    low2 = int(pieces[0].lstrip('hv'))
                    upp2 = int(pieces[1].lstrip('hv'))
                else:
                    low2 = int(bigPieces[1].lstrip('hv'))
                    upp2 = low2            
                if 'h' in bigPieces[0] and 'v' in bigPieces[1]:
                    ydim=abs(upp-low)+1
                    xdim=abs(upp2-low2)+1
                    speclist += self.spectrumBlock(firstspec,low, low2,ydim, xdim, dimension,orientation) + ','
                elif 'v' in bigPieces[0] and 'h' in bigPieces[1]:
                    xdim=abs(upp-low)+1
                    ydim=abs(upp2-low2)+1
                    speclist += self.spectrumBlock(firstspec,low2, low,nstrips, dimension, dimension,orientation)+ ','
                else:
                    print "error in mask, ignored:  " + x
            elif '>' in x:
                pieces = x.split('>')
                low = int(pieces[0].lstrip('hvs'))
                upp = int(pieces[1].lstrip('hvs'))
                if 'h' in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += self.spectrumBlock(firstspec,low, 0,nstrips, dimension, dimension,orientation)  + ','
                elif 'v' in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += self.spectrumBlock(firstspec,0,low, dimension, nstrips, dimension,orientation)  + ','
                else:
                    for i in range(low, upp + 1):
                        speclist += str(i) + ','
            elif 'h' in x:
                speclist += self.spectrumBlock(firstspec,int(x.lstrip('h')), 0,1, dimension, dimension,orientation) + ','
            elif 'v' in x:
                speclist += self.spectrumBlock(firstspec,0,int(x.lstrip('v')), dimension, 1, dimension,orientation) + ','
            else:
                speclist += x.lstrip('s') + ','
        
        self.spec_list += speclist.rpartition(':')[0]

    def _spectrumBlock(self, base, ylow, xlow, ydim, xdim, det_dimension, orientation):
        '''
            Compile a list of spectrum IDs for rectangular block of size xdim by ydim
        '''
        output = ''
        if orientation == Orientation.Horizontal:
            start_spec = base + ylow*det_dimension + xlow
            for y in range(0, ydim):
                for x in range(0, xdim):
                    output += str(start_spec + x + (y*det_dimension)) + ','
        elif orientation == Orientation.Vertical:
            start_spec = base + xlow*det_dimension + ylow
            for x in range(det_dimension - 1, det_dimension - xdim-1,-1):
                for y in range(0, ydim):
                    std_i = start_spec + y + ((det_dimension-x-1)*det_dimension)
            output += str(std_i ) + ','
        elif orientation == Orientation.Rotated:
            # This is the horizontal one rotated so need to map the xlow and vlow to their rotated versions
            start_spec = base + ylow*det_dimension + xlow
            max_spec = det_dimension*det_dimension + base - 1
            for y in range(0, ydim):
                for x in range(0, xdim):
                    std_i = start_spec + x + (y*det_dimension)
                    output += str(max_spec - (std_i - base)) + ','
        elif orientation == Orientation.HorizontalFlipped:
            start_spec = base + ylow*det_dimension + xlow
        for y in range(0,ydim):
            max_row = base + (y+1)*det_dimension - 1
            min_row = base + (y)*det_dimension
            for x in range(0,xdim):
                std_i = start_spec + x + (y*det_dimension)
            diff_s = std_i - min_row
            output += str(max_row - diff_s) + ','
    
        return output.rstrip(",")

    def _mask_phi(self, id, centre, phimin, phimax, use_mirror=True):
        '''
            Mask the detector bank such that only the region specified in the
            phi range is left unmasked
        '''
        # convert all angles to be between 0 and 360
        while phimax > 360 : phimax -= 360
        while phimax < 0 : phimax += 360
        while phimin > 360 : phimin -= 360
        while phimin < 0 : phimin += 360
        while phimax<phimin : phimax += 360
    
        #Convert to radians
        phimin = math.pi*phimin/180.0
        phimax = math.pi*phimax/180.0
        
        id = str(id)
        self._lim_phi_xml = (
            self._infinite_cylinder(id+'_plane1',centre, [math.cos(-phimin + math.pi/2.0),math.sin(-phimin + math.pi/2.0),0])
            + self._infinite_cylinder(id+'_plane2',centre, [-math.cos(-phimax + math.pi/2.0),-math.sin(-phimax + math.pi/2.0),0])
            + self._infinite_cylinder(id+'_plane3',centre, [math.cos(-phimax + math.pi/2.0),math.sin(-phimax + math.pi/2.0),0])
            + self._infinite_cylinder(id+'_plane4',centre, [-math.cos(-phimin + math.pi/2.0),-math.sin(-phimin + math.pi/2.0),0]))
        
        if use_mirror : 
            self._lim_phi_xml += '<algebra val="#((pla pla2):(pla3 pla4))" />'
        else:
            #the formula is different for acute verses obstruse angles
            if phimax-phimin > math.pi :
              # to get an obtruse angle, a wedge that's more than half the area, we need to add the semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#(pla:pla2)" />'
            else :
              # an acute angle, wedge is more less half the area, we need to use the intesection of those semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#(pla pla2)" />'

    def _normalizePhi(self, phi):
        if phi > 90.0:
            phi -= 180.0
        elif phi < -90.0:
            phi += 180.0
        else:
            pass
        return phi

    def set_phi_limit(self, phimin, phimax, phimirror):
        if phimirror :
            if phimin > phimax:
                phimin, phimax = phimax, phimin
            if abs(phimin) > 180.0 :
                phimin = -90.0
            if abs(phimax) > 180.0 :
                phimax = 90.0
        
            if phimax - phimin == 180.0 :
                phimin = -90.0
                phimax = 90.0
            else:
                phimin = self.normalizePhi(phimin)
                phimax = self.normalizePhi(phimax)
    
        self._mask_phi('unique phi', [0,0,0], phimin,phimax,phimirror)

    def execute(self, reducer, workspace):
        #set up the spectra lists and shape xml to mask
        detector = reducer.instrument.getDetector('rear')
        #rear specific masking
        self._ConvertToSpecList(self._specmask_r, detector)
        #masking for both detectors
        self._ConvertToSpecList(self._specmask, detector)
        #Time mask
        SANSUtility.MaskByBinRange(workspace,self._timemask_r)
        SANSUtility.MaskByBinRange(workspace,self._timemask)

        detector = reducer.instrument.getDetector('front')
        #front specific masking
        self._ConvertToSpecList(self._specmask_f, detector)
        #masking for both detectors
        self._ConvertToSpecList(self._specmask, detector)
        #Time mask
        SANSUtility.MaskByBinRange(workspace,self._timemask_f)
        SANSUtility.MaskByBinRange(workspace,self._timemask)

        if DEL__FINDING_CENTRE_ == True:
            if ( not self._min_radius is None) and (self._min_radius > 0.0):
                self.add_cylinder('center_find_beam_cen', self._min_radius, self._maskpt_rmin[0], self._maskpt_rmin[1])
            if ( not self._max_radius is None) and (self._max_radius > 0.0):
                self.add_outside_cylinder('center_find_beam_cen', self._max_radius, self._maskpt_rmin[0], self._maskpt_rmin[1])
        else:
            if ( not self._min_radius is None) and (self._min_radius > 0.0):
                self.add_cylinder('beam_stop', self._min_radius, self._maskpt_rmin[0], self._maskpt_rmin[1])
            xcenter = reducer.data_loader.maskpt_rmax[0]
            ycentre = reducer.data_loader.maskpt_rmax[1]
            if ( not self._max_radius is None) and (self._max_radius > 0.0):
                self.add_outside_cylinder('beam_area', self._max_radius, xcentre, ycentre)
        #now do the masking
        SANSReductionSteps.Mask.execute(self, reducer, workspace)

        if self._lim_phi_xml != '':
            MaskDetectorsInShape(workspace, self._lim_phi_xml)
            
    def __str__(self):
        return '    radius', self.min_radius, self.max_radius+'\n'+\
            '    global spectrum mask: ', str(self._specmask)+'\n'+\
            '    rear spectrum mask: ', str(self._specmask_r)+'\n'+\
            '    front spectrum mask: ', str(self._specmask_f)+'\n'+\
            '    global time mask: ', str(self._timemask)+'\n'+\
            '    rear time mask: ', str(self._timemask_r)+'\n'+\
            '    front time mask: ', str(self._timemask_f)+'\n'


class LoadSample(LoadRun):   
    """
    """
    #TODO: we don't need a dictionary here
    PERIOD_NOS = { "SCATTER_SAMPLE":1, "SCATTER_CAN":1 }

    def __init__(self, sample_run=None, reload=True, period=-1):
        super(LoadRun, self).__init__()
        self.SCATTER_SAMPLE = None
        self._SAMPLE_SETUP = None
        self._SAMPLE_RUN = None
        self._SAMPLE_N_PERIODS = -1
        self._sample_run = sample_run
        self._reload = reload
        self._period = period
        
        self.maskpt_rmin = None
        self.maskpt_rmax = None
        
        #This is set to the name of the workspace that was loaded, with some changes made to it 
        self.uncropped = None
    
    def set_options(self, reload=True, period=-1):
        self._reload = reload
        self._period = period
        
    def execute(self, reducer, workspace):
        # If we don't have a data file, look up the workspace handle
        if self._sample_run is None:
            self._sample_run = reducer._data_files.values()[0]
        # Code from AssignSample
        self._clearPrevious(self.SCATTER_SAMPLE)
        self._SAMPLE_N_PERIODS = -1
        
        if( self._sample_run.startswith('.') or self._sample_run == '' or self._sample_run == None):
            self._SAMPLE_SETUP = None
            self._SAMPLE_RUN = ''
            self.SCATTER_SAMPLE = None
            return '', '()'

        self.SCATTER_SAMPLE, reset, logname, filepath, self._SAMPLE_N_PERIODS = self._assignHelper(self._sample_run, False, self._reload, self._period, reducer)
        if self.SCATTER_SAMPLE.getName() == '':
            _issueWarning('Unable to load SANS sample run, cannot continue.')
            return '','()'
        if reset == True:
            self._SAMPLE_SETUP = None

        self.uncropped  = self.SCATTER_SAMPLE.getName()
        p_run_ws = mtd[self.uncropped ]
        run_num = p_run_ws.getSampleDetails().getLogData('run_number').value()
        reducer.instrument.set_up_for_run(run_num)

        try:
            logvalues = reducer.instrument.load_detector_logs(logname,filepath)
            if logvalues == None:
                mtd.deleteWorkspace(self.SCATTER_SAMPLE.getName())
                _issueWarning("Sample logs cannot be loaded, cannot continue")
                return '','()'
        except AttributeError:
            if not reducer.instrument.name() == 'LOQ': raise
        
        self.PERIOD_NOS["SCATTER_SAMPLE"] = self._period
    
        if (reducer.instrument.name() == 'LOQ'):
            return self.SCATTER_SAMPLE.getName(), None
    
        reducer.instrument.FRONT_DET_Z = float(logvalues['Front_Det_Z'])
        reducer.instrument.FRONT_DET_X = float(logvalues['Front_Det_X'])
        reducer.instrument.FRONT_DET_ROT = float(logvalues['Front_Det_Rot'])
        reducer.instrument.REAR_DET_Z = float(logvalues['Rear_Det_Z'])
        reducer.instrument.REAR_DET_X = float(logvalues['Rear_Det_X'])
        # End of AssignSample

        # Put the components in the correct positions
        beamcoords = reducer._beam_finder.get_beam_center()
        self.maskpt_rmin, self.maskpt_rmax = reducer.instrument.set_component_positions(self.SCATTER_SAMPLE.getName(), beamcoords[0], beamcoords[1])
        # SANSReduction._init_run. This should go in LoadSample
        #sample_setup = _init_run(SCATTER_SAMPLE, beam_center, False)    
        mantid.sendLogMessage('::SANS:: Initialized sample workspace to [' + str(beamcoords[0]) + ',' + str(beamcoords[1]) + ']' )

        # Get the detector bank that is to be used in this analysis leave the complete workspace
        CropWorkspace(self.uncropped, workspace,
            StartWorkspaceIndex = reducer.instrument.cur_detector().first_spec_num - 1,
            EndWorkspaceIndex = reducer.instrument.cur_detector().last_spec_num - 1)

        # Create a run details object
        TRANS_SAMPLE = ''
        DIRECT_SAMPLE = ''
        if reducer._transmission_calculator is not None:
            TRANS_SAMPLE = reducer._transmission_calculator.TRANS_SAMPLE
        if reducer._transmission_calculator is not None:
            DIRECT_SAMPLE = reducer._transmission_calculator.DIRECT_SAMPLE
            
        sample_setup = SANSUtility.RunDetails(self.SCATTER_SAMPLE, workspace, 
                                              TRANS_SAMPLE, DIRECT_SAMPLE, self.maskpt_rmin, self.maskpt_rmax, 'sample')
        # End of _init_run
        #TODO: 
        sample_setup.setReducedWorkspace(workspace)

def extract_workspace_name(run_string, is_trans=False, prefix='', run_number_width=8):
    pieces = run_string.split('.')
    if len(pieces) != 2 :
         raise RuntimeError, "Invalid run specified: " + run_string + ". Please use RUNNUMBER.EXT format"
    else:
        run_no = pieces[0]
        ext = pieces[1]
    
    fullrun_no, logname, shortrun_no = _padRunNumber(run_no, run_number_width)

    if is_trans:
        wkspname =  shortrun_no + '_trans_' + ext.lower()
    else:
        wkspname =  shortrun_no + '_sans_' + ext.lower()
    
    return wkspname, run_no, logname, prefix+fullrun_no+'.'+ext

def _padRunNumber(run_no, field_width):
    nchars = len(run_no)
    digit_end = 0
    for i in range(0, nchars):
        if run_no[i].isdigit():
            digit_end += 1
        else:
            break
    
    if digit_end == nchars:
        filebase = run_no.rjust(field_width, '0')
        return filebase, filebase, run_no
    else:
        filebase = run_no[:digit_end].rjust(field_width, '0')
        return filebase + run_no[digit_end:], filebase, run_no[:digit_end]


class UnitsConvert(ReductionStep):
    def __init__(self, units, w_low = None, w_step = None, w_high = None):
        #TODO: data_file = None only makes sense when AppendDataFile is used... (AssignSample?)
        super(UnitsConvert, self).__init__()
        self._units = units
        self.wav_low = w_low
        self.wav_high = w_high
        self.wav_step = w_step

    def execute(self, reducer, workspace, rebin_alg = 'use default'):
        ConvertUnits(workspace, workspace, self._units)
        
        if rebin_alg == 'use default':
            rebin_alg = 'Rebin' 
        rebin_com = rebin_alg+'(workspace, workspace, "'+self.get_rebin()+'")'
        eval(rebin_com)

    def get_rebin(self):
        return str(self.wav_low)+', ' + str(self.wav_step) + ', ' + str(self.wav_high)
    
    def set_rebin(self, w_low = None, w_step = None, w_high = None):
        if not w_low is None:
            self.wav_low = float(w_low)
        if not w_step is None:
            self.wav_step = float(w_step)
        if not w_high is None:
            self.wav_high = float(w_high)

    def get_range(self):
        return str(self.wav_low)+'_'+str(self.wav_high)

    def set_range(self, w_low = None, w_high = None):
        self.set_rebin(w_low, None, w_high)

    def __str__(self):
        return '    Wavelength range: ' + self.get_rebin()

class ConvertToQ(ReductionStep):
    def __init__(self):
        #TODO: data_file = None only makes sense when AppendDataFile is used... (AssignSample?)
        super(ConvertToQ, self).__init__()
        
        #this should be set to 1D or 2D
        self._output_type = None
        self._output_types = {'1D' : 'Q1D', '2D': 'Qxy'}
        #if true gravity is taken into account in the Q1D calculation
        self._use_gravity = False
    
    def set_output_type(self, discript):
        self._output_type = self._output_types[discript]
        
    def set_gravity(self, flag):
        if isinstance(flag, bool) or isinstance(flag, int):
            self._use_gravity = bool(flag)
        else:
            _issueWarning("Invalid GRAVITY flag passed, try True/False. Setting kept as " + str(self._use_gravity)) 
                   
    def execute(self, reducer, workspace):
        #Steve, I'm not sure this contains good error values 
        errorsWS = reducer.norm_mon.prenormed
        if self._output_type == 'Q1D':
            Q1D(workspace, errorsWS, workspace, reducer.Q_REBIN, AccountForGravity=self._use_gravity)
            ReplaceSpecialValues(workspace, workspace, NaNValue="0", InfinityValue="0")
            rem_zeros = SANSReductionSteps.StripEndZeros()
            rem_zeros.execute(reducer, workspace)

        elif self._output_type == 'Qxy':
            Qxy(workspace, workspace, reducer.QXY2, reducer.DQXY)
            ReplaceSpecialValues(workspace, workspace, NaNValue="0", InfinityValue="0")
        else:
            raise NotImplementedError('The type of Q reduction hasn''t been set, e.g. 1D or 2D')

class NormalizeToMonitor(SANSReductionSteps.Normalize):
    """
        This step performs background removal on the monitor spectrum
        used for normalization and, for LOQ runs, executes a LOQ specific
        correction. It's input workspace is copied and accessible later
        as prenomed 
    """
    def __init__(self):
        super(NormalizeToMonitor, self).__init__()
        #Steve is unsure about why this is need. It is used later in the calculation of errors on Q
        self.prenormed = None

    def execute(self, reducer, workspace):
        # At this point need to fork off workspace name to keep a workspace containing raw counts
        self.prenormed = 'to_delete_'+workspace+'_prenormed'
        RenameWorkspace(workspace, self.prenormed)

        if self._normalization_spectrum is None:
            self._normalization_spectrum = reducer.instrument.get_incident_mon()
        
        mtd.sendLogMessage('::SANS::Normalizing to monitor ' + str(self._normalization_spectrum))
        # Get counting time or monitor
        norm_ws = workspace+"_normalization"
        norm_ws = 'Monitor'
        spec_index = self._normalization_spectrum-1

        CropWorkspace(reducer.data_loader.uncropped, norm_ws,
                      StartWorkspaceIndex = spec_index, 
                      EndWorkspaceIndex   = spec_index)
    
        if reducer.instrument.name() == 'LOQ':
            RemoveBins(norm_ws, norm_ws, '19900', '20500',
                Interpolation="Linear")
        
        # Remove flat background
        if reducer.BACKMON_START != None and reducer.BACKMON_END != None:
            FlatBackground(norm_ws, norm_ws, StartX = reducer.BACKMON_START,
                EndX = reducer.BACKMON_END, WorkspaceIndexList = '0')
    
        #perform the sample conversion on the monitor spectrum as was applied to the workspace
        if reducer.instrument.is_interpolating_norm():
            rebin_alg = 'InterpolatingRebin'
        else :
            rebin_alg = 'use default'
        reducer.to_wavelen.execute(reducer, norm_ws, rebin_alg)

        Divide(self.prenormed, norm_ws, workspace)

        mtd.deleteWorkspace(norm_ws)

# Setup the transmission workspace
##
class CalculateTransmission(ReductionStep):
    def __init__(self, run_setup, use_def_trans):
        super(CalcTransmissionCorr, self).__init__()
    
    def execute(self, reducer, workspace):
        trans_raw = run_setup.getTransRaw()
        direct_raw = run_setup.getDirectRaw()
        if trans_raw == '' or direct_raw == '':
            return None
    
        if use_def_trans == DefaultTrans:
            wavbin = str(TRANS_WAV1_FULL) 
            wavbin = + ',' + str(ReductionSingleton().to_wavelen.wav_step)
            wavbin = + ',' + str(TRANS_WAV2_FULL)
            translambda_min = TRANS_WAV1_FULL
            translambda_max = TRANS_WAV2_FULL
        else:
            translambda_min = TRANS_WAV1
            translambda_max = TRANS_WAV2
            wavbin = str(Reducer.to_wavelen.get_rebin())
    
        fittedtransws = trans_raw.split('_')[0] + '_trans_' + run_setup.getSuffix() + '_' + str(translambda_min) + '_' + str(translambda_max)
        unfittedtransws = fittedtransws + "_unfitted"
        if use_def_trans == False or \
        (TRANS_FIT != 'Off' and mtd.workspaceExists(fittedtransws) == False) or \
        (TRANS_FIT == 'Off' and mtd.workspaceExists(unfittedtransws) == False):
            # If no fitting is required just use linear and get unfitted data from CalculateTransmission algorithm
            if TRANS_FIT == 'Off':
                fit_type = 'Linear'
            else:
                fit_type = TRANS_FIT
            #retrieve the user setting that tells us whether Rebin or InterpolatingRebin will be used during the normalisation 
            if reducer.instrument.name() == 'LOQ':
                # Change the instrument definition to the correct one in the LOQ case
                LoadInstrument(trans_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
                LoadInstrument(direct_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
                
                trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw,
                    '1,2', BACKMON_START, BACKMON_END, wavbin, 
                    reducer.instrument.use_interpol_trans_calc, True)
                
                direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw,
                    '1,2', BACKMON_START, BACKMON_END, wavbin,
                    reducer.instrument.use_interpol_trans_calc, True)
                
                CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws, MinWavelength = translambda_min, MaxWavelength =  translambda_max, \
                                      FitMethod = fit_type, OutputUnfittedData=True)
            else:
                trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw,
                    '1,2', BACKMON_START, BACKMON_END, wavbin,
                    reducer.instrument.use_interpol_trans_calc, False)
                
                direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw,
                    '1,2', BACKMON_START, BACKMON_END, wavbin,
                    reducer.instrument.use_interpol_trans_calc, False)
                
                CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws,
                    reducer.instrument.incid_mon_4_trans_calc, reducer.instrument.trans_monitor,
                    MinWavelength = translambda_min, MaxWavelength = translambda_max,
                    FitMethod = fit_type, OutputUnfittedData=True)
            # Remove temporaries
            mantid.deleteWorkspace(trans_tmp_out)
            mantid.deleteWorkspace(direct_tmp_out)
            
        if TRANS_FIT == 'Off':
            result = unfittedtransws
            mantid.deleteWorkspace(fittedtransws)
        else:
            result = fittedtransws
    
        if self.use_def_trans == DefaultTrans:
            tmp_ws = 'trans_' + run_setup.getSuffix() + '_' + reducer.to_wavelen.get_range()
            CropWorkspace(result, tmp_ws, XMin = str(reducer.to_wavelen.wav_low), XMax = str(reducer.to_wavelen.wav_high))
            trans_ws = tmp_ws
        else: 
            trans_ws = result

        Divide(workspace, trans_ws, workspace)

class ISISCorrections(SANSReductionSteps.CorrectToFileStep):
    def __init__(self, corr_type = '', operation = ''):
        super(ISISCorrections, self).__init__('', "Wavelength", "Divide")

        # Scaling values [%]
        self.rescale= 100.0
    
    def set_filename(self, filename):
        raise AttributeError('The correction must be set in the instrument, or use the CorrectionToFileStep instead')

    def execute(self, reducer, workspace):
        #use the instrument's correction file
        self._filename = reducer.instrument.cur_detector().correction_file
        #do the correct to file
        super(ISISCorrections, self).execute(reducer, workspace)

        scalefactor = self.rescale
        # Data reduced with Mantid is a factor of ~pi higher than colette.
        # For LOQ only, divide by this until we understand why.
        if reducer.instrument.name() == 'LOQ':
            rescaleToColette = math.pi
            scalefactor /= rescaleToColette

        ws = mtd[workspace]
        ws *= scalefactor

