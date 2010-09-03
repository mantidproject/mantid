"""
    Implementation of reduction steps for ISIS SANS instruments
    
    Most of this code is a copy-paste from SANSReduction.py, organized to be used with
    ReductionStep objects. The guts needs refactoring.
"""
from Reducer import ReductionStep
from SANSReductionSteps import *
from mantidsimple import *
import SANSUtility
import SANSInsts
import os
import math

def _issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    mantid.sendLogMessage('::SANS::Warning: ' + msg)
    
class Transmission(BaseTransmission):
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
        super(Transmission, self).__init__()
    
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
        _clearPrevious(self.TRANS_SAMPLE)
        _clearPrevious(self.DIRECT_SAMPLE)
        
        if self._trans_sample not in [None, '']:
            trans_ws, dummy1, dummy2, dummy3, self.TRANS_SAMPLE_N_PERIODS = \
                _assignHelper(self._trans_sample, True, self._sample_reload, self._sample_period, reducer)
            self.TRANS_SAMPLE = trans_ws.getName()
        
        if self._direct_sample not in [None, '']:
            direct_sample_ws, dummy1, dummy2, dummy3, self.DIRECT_SAMPLE_N_PERIODS = \
                _assignHelper(self._direct_sample, True, self._sample_reload, self._sample_period, reducer)
            self.DIRECT_SAMPLE = direct_sample_ws.getName()
        
        # Load transmission can
        _clearPrevious(self.TRANS_CAN)
        _clearPrevious(self.DIRECT_CAN)
    
        if self._trans_can not in [None, '']:
            can_ws, dummy1, dummy2, dummy3, self.TRANS_CAN_N_PERIODS = \
                _assignHelper(self._trans_can, True, self._can_reload, self._can_period, reducer)
            self.TRANS_CAN = can_ws.getName()
            
        if self._direct_can in [None, '']:
            self.DIRECT_CAN, self.DIRECT_CAN_N_PERIODS = self.DIRECT_SAMPLE, self.DIRECT_SAMPLE_N_PERIODS
        else:
            direct_can_ws, dummy1, dummy2, dummy3, self.DIRECT_CAN_N_PERIODS = \
                _assignHelper(self._direct_can, True, self._can_reload, self._can_period, reducer)
            self.DIRECT_CAN = direct_can_ws.getName()
 
        #TODO: Call CalculateTransmission

        
class CanSubtraction(ReductionStep):
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
        
        _clearPrevious(self.SCATTER_CAN)
        
        self._CAN_N_PERIODS = -1
        
        if( can_run.startswith('.') or can_run == '' or can_run == None):
            self.SCATTER_CAN.reset()
            self._CAN_RUN = ''
            self._CAN_SETUP = None
            return '', '()'
    
        self._CAN_RUN = can_run
        self.SCATTER_CAN ,reset, logname,filepath, self._CAN_N_PERIODS = \
            _assignHelper(can_run, False, reload, period, reducer)
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
        smp_values.append(reducer.instrument.FRONT_DET_Z + reducer.instrument.FRONT_DET_Z_CORR)
        smp_values.append(reducer.instrument.FRONT_DET_X + reducer.instrument.FRONT_DET_X_CORR)
        smp_values.append(reducer.instrument.FRONT_DET_ROT + reducer.instrument.FRONT_DET_ROT_CORR)
        smp_values.append(reducer.instrument.REAR_DET_Z + reducer.instrument.REAR_DET_Z_CORR)
        smp_values.append(reducer.instrument.REAR_DET_X + reducer.instrument.REAR_DET_X_CORR)
    
        # Check against sample values and warn if they are not the same but still continue reduction
        if len(logvalues) == 0:
            return  self.SCATTER_CAN.getName(), logvalues
        
        can_values = []
        can_values.append(float(logvalues['Front_Det_Z']) + reducer.instrument.FRONT_DET_Z_CORR)
        can_values.append(float(logvalues['Front_Det_X']) + reducer.instrument.FRONT_DET_X_CORR)
        can_values.append(float(logvalues['Front_Det_Rot']) + reducer.instrument.FRONT_DET_ROT_CORR)
        can_values.append(float(logvalues['Rear_Det_Z']) + reducer.instrument.REAR_DET_Z_CORR)
        can_values.append(float(logvalues['Rear_Det_X']) + reducer.instrument.REAR_DET_X_CORR)
    
    
        det_names = ['Front_Det_Z', 'Front_Det_X','Front_Det_Rot', 'Rear_Det_Z', 'Rear_Det_X']
        for i in range(0, 5):
            if math.fabs(smp_values[i] - can_values[i]) > 5e-04:
                mantid.sendLogMessage("::SANS::Warning: values differ between sample and can runs. Sample = " + str(smp_values[i]) + \
                                  ' , Can = ' + str(can_values[i]))
                reducer.instrument.append_marked(det_names[i])
        
        return self.SCATTER_CAN.getName(), logvalues

    def execute(self, reducer, workspace):
        """
        """ 
        if self._can_run is not None:
            self._assign_can(reducer, self._can_run, reload = self._can_run_reload, period = self._can_run_period)
        
        # Apply same corrections as for data then subtract from data
    
class Mask(ReductionStep):
    """
    """
    def __init__(self, timemask='', timemask_r='', timemask_f='', 
                 specmask='', specmask_r='', specmask_f=''):
        """
            ISIS mask
        """
        self._timemask=timemask 
        self._timemask_r=timemask_r
        self._timemask_f=timemask_f
        self._specmask=specmask
        self._specmask_r=specmask_r
        self._specmask_f=specmask_f
        
    def execute(self, reducer, workspace):
        return NotImplemented
    
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
     
class LoadSample(ReductionStep):   
    """
    """
    SCATTER_SAMPLE = None
    _SAMPLE_SETUP = None
    _SAMPLE_RUN = None
    _SAMPLE_N_PERIODS = -1
    #TODO: we don't need a dictionary here
    PERIOD_NOS = { "SCATTER_SAMPLE":1, "SCATTER_CAN":1 }
    
    def __init__(self, sample_run=None, reload=True, period=-1):
        self._sample_run = sample_run
        self._reload = reload
        self._period = period
    
    def set_options(self, reload=True, period=-1):
        self._reload = reload
        self._period = period        
        
    def execute(self, reducer, workspace):
        # If we don't have a data file, look up the workspace handle
        if self._sample_run is None:
            if workspace in reducer._data_files:
                self._sample_run = reducer._data_files[workspace]
            else:
                raise RuntimeError, "ISISReductionSteps.LoadSample doesn't recognize workspace handle %s" % workspace        
        
        _clearPrevious(self.SCATTER_SAMPLE)
        self._SAMPLE_N_PERIODS = -1
        
        if( self._sample_run.startswith('.') or self._sample_run == '' or self._sample_run == None):
            self._SAMPLE_SETUP = None
            self._SAMPLE_RUN = ''
            self.SCATTER_SAMPLE = None
            return '', '()'
        
        self._SAMPLE_RUN = self._sample_run
        self.SCATTER_SAMPLE, reset, logname, filepath, self._SAMPLE_N_PERIODS = _assignHelper(self._sample_run, False, self._reload, self._period, reducer)
        if self.SCATTER_SAMPLE.getName() == '':
            _issueWarning('Unable to load SANS sample run, cannot continue.')
            return '','()'
        if reset == True:
            self._SAMPLE_SETUP = None
    
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
    
        return self.SCATTER_SAMPLE.getName(), logvalues

    
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
        
        if os.path.splitext(self._data_file)[1].lower().startswith('n'):
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
        
        reducer.opt_steps['SampleGeomCor'] = SampleGeomCor()
        #set the defaults incase the following line fails
        reducer.opt_steps['SampleGeomCor'].set_dimensions_to_unity()
        reducer.opt_steps['SampleGeomCor'].read_from_workspace(workspace)
        
        return [ os.path.dirname(fullpath), workspace, numPeriods]        
        

#class DirectBeamTransmission(BaseTransmission):
#    """
#        Calculate transmission using the direct beam method
#    """
#    def __init__(self):
#        super(DirectBeamTransmission, self).__init__()
#        # Location of the data files used to calculate transmission
#        ## Transmission workspace (output of transmission calculation)
#        self.transmission_run = None
#        self.can_run = None
#        self.transmission_can = None
#        self.direct = None
#        self.direct_can = None
#        
#    def execute(self, reducer, workspace=None):
#        """
#        """
##        if not self.transmission_run is None:
#        pass
        
# Helper function
def _assignHelper(run_string, is_trans, reload = True, period = -1, reducer=None):
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

def _clearPrevious(inWS, others = []):
    if inWS != None:
        if type(inWS) == SANSUtility.WorkspaceDetails:
            inWS = inWS.getName()
        if mtd.workspaceExists(inWS) and (not inWS in others):
            mtd.deleteWorkspace(inWS)
            
            
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


