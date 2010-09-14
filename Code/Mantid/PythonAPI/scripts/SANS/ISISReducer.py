"""
    ISIS-specific implementation of the SANS Reducer. 
    
    WARNING: I'm still playing around with the ISIS reduction to try to 
    understand what's happening and how best to fit it in the Reducer design. 
     
"""
from SANSReducer import SANSReducer
from Reducer import ReductionStep
import SANSReductionSteps
import ISISReductionSteps
import SANSUtility
from mantidsimple import *

## Version number
__version__ = '0.0'

def _issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    mantid.sendLogMessage('::SANS::Warning: ' + msg)
    
class ISISReducer(SANSReducer):
    """
        ISIS Reducer
        TODO: need documentation for all the data member
        TODO: need to see whether all those data members really belong here
    """
    RMIN = None
    RMAX = None
    DEF_RMIN = None
    DEF_RMAX = None
    
    WAV1 = None
    WAV2 = None
    DWAV = None
    Q_REBIN = None
    QXY2 = None
    DQY = None
    
    # Scaling values [%]
    RESCALE = 100.0
    
    BACKMON_START = None
    BACKMON_END = None
    
    lowAngDetSet = True
    
    DIRECT_BEAM_FILE_R = None
    DIRECT_BEAM_FILE_F = None  
    
    # Component positions
    PHIMIN=-90.0
    PHIMAX=90.0
    PHIMIRROR=True
    
    CORRECTION_TYPE = '1D'
    DIMENSION = None
    SPECMIN = None
    SPECMAX = None

    
    ## Flag for gravity correction
    # Belongs in Q1D
    _use_gravity = False
    
    ## Path for user settings files
    _user_path = '.'
    
    def __init__(self):
        super(ISISReducer, self).__init__()
        self._transmission_calculator = ISISReductionSteps.Transmission()
        # Default data loader
        self._data_loader = ISISReductionSteps.LoadSample()
        self.geometry_correcter = SANSReductionSteps.SampleGeomCor()
        
        self._final_workspace = ''
        
    def set_user_path(self, path):
        """
            Set the path for user files
            @param path: user file path
        """
        if os.path.isdir(path):
            self._user_path = path
        else:
            raise RuntimeError, "ISISReducer.set_user_path: provided path is not a directory (%s)" % path

    def load_set_options(self, reload=True, period=-1):
        if not issubclass(self._data_loader.__class__, ISISReductionSteps.LoadSample):
            raise RuntimeError, "ISISReducer.load_set_options: method called with wrong loader class"
        self._data_loader.set_options(reload, period)

    def append_data_file(self, data_file, workspace=None):
        """
            Append a file to be processed.
            @param data_file: name of the file to be processed
            @param workspace: optional name of the workspace for this data,
                default will be the name of the file 
        """
        wrkspc, _, _, filename = ISISReductionSteps.extract_workspace_name(data_file, False, 
                                                                           self.instrument.name(), 
                                                                           self.instrument.run_number_width)
        if workspace is None:
            workspace = wrkspc
            
        self._full_file_path(filename)
        
        self._data_files[workspace] = data_file
        
    def set_background(self, can_run=None, reload = True, period = -1):
        """
            Sets the can data to be subtracted from sample data files
            @param data_file: Name of the can run file
        """
        if can_run is None:
            self._background_subtracter = None
        else:
            self._background_subtracter = ISISReductionSteps.CanSubtraction(can_run, reload=reload, period=period)

    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log"):
        if not issubclass(self._transmission_calculator.__class__, SANSReductionSteps.BaseTransmission):
            raise RuntimeError, "ISISReducer.set_trans_fit: transmission calculator not set"
        self._transmission_calculator.set_trans_fit(lambda_min, lambda_max, fit_method)
        
    def set_trans_sample(self, sample, direct, reload=True, period=-1):
        if not issubclass(self._transmission_calculator.__class__, SANSReductionSteps.BaseTransmission):
            raise RuntimeError, "ISISReducer.set_trans_sample: transmission calculator not set"
        self._transmission_calculator.set_trans_sample(sample, direct, reload, period)
        
    def set_trans_can(self, can, direct, reload = True, period = -1):
        if not issubclass(self._transmission_calculator.__class__, SANSReductionSteps.BaseTransmission):
            raise RuntimeError, "ISISReducer.set_trans_can: transmission calculator not set"
        self._transmission_calculator.set_trans_can(can, direct, reload, period)
        
    def set_gravity(self, flag):
        if isinstance(flag, bool) or isinstance(flag, int):
            self._use_gravity = flag
        else:
            _issueWarning("Invalid GRAVITY flag passed, try True/False. Setting kept as " + str(self._use_gravity)) 
                   
    def set_monitor_spectrum(self, specNum, interp=False):
        self.instrument.set_incident_mon(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if interp :
            self.instrument.set_interpolating_norm()
                        
    def suggest_monitor_spectrum(self, specNum, interp=False):
        self.instrument.suggest_incident_mntr(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if interp :
            self.instrument.suggest_interpolating_norm()
                    
    def set_trans_spectrum(self, specNum, interp=False):
        self.instrument.incid_mon_4_trans_calc = int(specNum)
        #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
        if interp :
            self.instrument.use_interpol_trans_calc = True                    
                      
    def set_phi_limit(self, phimin, phimax, phimirror=True):
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
                phimin = SANSUtility.normalizePhi(phimin)
                phimax = SANSUtility.normalizePhi(phimax)
              
        self.PHIMIN = phimin
        self.PHIMAX = phimax
        self.PHIMIRROR = phimirror
        
    def _initialize_mask(self):
        self.instrument.FRONT_DET_Z_CORR = 0.0
        self.instrument.FRONT_DET_Y_CORR = 0.0
        self.instrument.FRONT_DET_X_CORR = 0.0
        self.instrument.FRONT_DET_ROT_CORR = 0.0
        self.instrument.REAR_DET_Z_CORR = 0.0
        self.instrument.REAR_DET_X_CORR = 0.0

        self.mask('MASK/CLEAR')
        self.mask('MASK/CLEAR/TIME')
        self.DIRECT_BEAM_FILE_F = None
        self.DIRECT_BEAM_FILE_R = None

        self.RMIN = None
        self.RMAX = None
        self.DEF_RMIN = None
        self.DEF_RMAX = None
       
        self.WAV1 = None
        self.WAV2 = None
        self.DWAV = None
        self.Q_REBIN = None
        self.QXY = None
        self.DQY = None
         
        self.BACKMON_END = None
        self.BACKMON_START = None
        
        if self.geometry_correcter is not None:
            self.geometry_correcter = SANSReductionSteps.SampleGeomCor()
        
        self.RESCALE = 100.0

    def mask(self, instruction):
        if self._mask is None:
            masker = ISISReductionSteps.Mask()
            self.set_mask(masker)
        self._mask.parse_instruction(instruction)
    
    def set_wavelength_range(self, start, end):
        self.WAV1 = start
        self.WAV2 = end
        
        
    def reduce(self):
        """
            Go through the list of reduction steps
        """
        # Check that an instrument was specified
        if self.instrument is None:
            raise RuntimeError, "Reducer: trying to run a reduction without an instrument specified"

        # Go through the list of steps that are common to all data files
        self.pre_process()

        #TODO: change the following
        finding_centre = False

        # Go through the list of files to be reduced
        for file_ws in self._data_files:
            
            #TODO: set up the final workspace name
            if finding_centre == False:
                self._final_workspace = file_ws + '_' + str(self.WAV1) + '_' + str(self.WAV2)
            else:
                self._final_workspace = file_ws.split('_')[0] + '_quadrants'
            #self._data_files[final_workspace] = self._data_files[file_ws]
            #del self._data_files[file_ws]
            #----> can_setup.setReducedWorkspace(tmp_can)
            
            # Perform correction
            #TODO: all reduction steps from Correct() should go in _2D_steps()
            #Correct(sample_setup, wav_start, wav_end, use_def_trans, finding_centre)
            for item in self._reduction_steps:
                item.execute(self, file_ws)    
                     
                 
            # Crop Workspace to remove leading and trailing zeroes
            #TODO: deal with this once we have the final workspace name sorted out
            if False:
                if finding_centre == False:
                    # Replaces NANs with zeroes
                    ReplaceSpecialValues(InputWorkspace = self._final_workspace, OutputWorkspace = self._final_workspace, NaNValue="0", InfinityValue="0")
                    if self.CORRECTION_TYPE == '1D':
                        SANSUtility.StripEndZeroes(self._final_workspace)
                    # Store the mask file within the final workspace so that it is saved to the CanSAS file
                    AddSampleLog(self._final_workspace, "UserFile", self.MASKFILE)
                else:
                    quadrants = {1:'Left', 2:'Right', 3:'Up',4:'Down'}
                    for key, value in quadrants.iteritems():
                        old_name = self._final_workspace + '_' + str(key)
                        RenameWorkspace(old_name, value)
                        AddSampleLog(value, "UserFile", self._maskfile)       
                        
    def pre_process(self): 
        """
            Reduction steps that are meant to be executed only once per set
            of data files. After this is executed, all files will go through
            the list of reduction steps.
        """
        # from SANSReduction._initReduction
        self.DIMENSION, self.SPECMIN, self.SPECMAX  = SANSUtility.GetInstrumentDetails(self.instrument)
        # End of _initReduction    

        # Create the list of reduction steps
        self._to_steps()  
    
        
    def read_mask_file(self, filename):
        """
            Reads a SANS mask file
            
            TODO: filename now really expects a file path. Need to use a default directory to look into
            
            @param filename: file path of the mask file to be read
        """
        self._maskfile = filename
        #Check that the file exists.
        if not os.path.isfile(filename):
            raise RuntimeError, "Cannot read mask. File path '%s' does not exist." % filename
            
        # Re-initializes default values
        self._initialize_mask()
    
        file_handle = open(filename, 'r')
        for line in file_handle:
            if line.startswith('!'):
                continue
            # This is so that I can be sure all EOL characters have been removed
            line = line.lstrip().rstrip()
            upper_line = line.upper()
            if upper_line.startswith('L/'):
                self._readLimitValues(line)
            
            elif upper_line.startswith('MON/'):
                self._readMONValues(line)
            
            elif upper_line.startswith('MASK'):
                self.mask(upper_line)
            
            elif upper_line.startswith('SET CENTRE'):
                values = upper_line.split()
                self.set_beam_finder(SANSReductionSteps.BaseBeamFinder(float(values[2])/1000.0, float(values[3])/1000.0))
            
            elif upper_line.startswith('SET SCALES'):
                values = upper_line.split()
                self.RESCALE = float(values[2]) * 100.0
            
            elif upper_line.startswith('SAMPLE/OFFSET'):
                values = upper_line.split()
                self.instrument.set_sample_offset(values[1])
            
            elif upper_line.startswith('DET/'):
                type = upper_line[4:]
                if type.startswith('CORR'):
                    self._readDetectorCorrections(upper_line[8:])
                else:
                    # This checks whether the type is correct and issues warnings if it is not
                    self.instrument.setDetector(type)
            
            elif upper_line.startswith('GRAVITY'):
                flag = upper_line[8:]
                if flag == 'ON':
                    self.set_gravity(True)
                elif flag == 'OFF':
                    self.set_gravity(False)
                else:
                    _issueWarning("Gravity flag incorrectly specified, disabling gravity correction")
                    self.set_gravity(False)
            
            elif upper_line.startswith('BACK/MON/TIMES'):
                tokens = upper_line.split()
                if len(tokens) == 3:
                    self.BACKMON_START = int(tokens[1])
                    self.BACKMON_END = int(tokens[2])
                else:
                    _issueWarning('Incorrectly formatted BACK/MON/TIMES line, not running FlatBackground.')
                    self.BACKMON_START = None
                    self.BACKMON_END = None
            
            elif upper_line.startswith("FIT/TRANS/"):
                params = upper_line[10:].split()
                if len(params) == 3:
                    fit_type, lambdamin, lambdamax = params
                    self._transmission_calculator.set_trans_fit(lambda_min=lambdamin, 
                                                                lambda_max=lambdamax, 
                                                                fit_method=fit_type)
                else:
                    _issueWarning('Incorrectly formatted FIT/TRANS line, setting defaults to LOG and full range')
                    self._transmission_calculator = ISISReductionSteps.Transmission()
            
            else:
                continue
    
        # Close the handle
        file_handle.close()
        # Check if one of the efficency files hasn't been set and assume the other is to be used
        if self.DIRECT_BEAM_FILE_R == None and self.DIRECT_BEAM_FILE_F != None:
            self.DIRECT_BEAM_FILE_R = self.DIRECT_BEAM_FILE_F
        if self.DIRECT_BEAM_FILE_F == None and self.DIRECT_BEAM_FILE_R != None:
            self.DIRECT_BEAM_FILE_F = self.DIRECT_BEAM_FILE_R
            
        # just print the name, remove the path
        filename = os.path.basename(filename)
        return True

            
    # Read a limit line of a mask file
    def _readLimitValues(self, limit_line):
        limits = limit_line.split('L/')
        if len(limits) != 2:
            _issueWarning("Incorrectly formatted limit line ignored \"" + limit_line + "\"")
            return
        limits = limits[1]
        limit_type = ''
        if not ',' in limit_line:
            # Split with no arguments defaults to any whitespace character and in particular
            # multiple spaces are include
            elements = limits.split()
            if len(elements) == 4:
                limit_type, minval, maxval, step = elements[0], elements[1], elements[2], elements[3]
                rebin_str = None
                step_details = step.split('/')
                if len(step_details) == 2:
                    step_size = step_details[0]
                    step_type = step_details[1]
                    if step_type.upper() == 'LIN':
                        step_type = ''
                    else:
                        step_type = '-'
                else:
                    step_size = step_details[0]
                    step_type = ''
            elif len(elements) == 3:
                limit_type, minval, maxval = elements[0], elements[1], elements[2]
            else:
                # We don't use the L/SP line
                if not 'L/SP' in limit_line:
                    _issueWarning("Incorrectly formatted limit line ignored \"" + limit_line + "\"")
                    return
        else:
            limit_type = limits[0].lstrip().rstrip()
            rebin_str = limits[1:].lstrip().rstrip()
            minval = maxval = step_type = step_size = None
    
        if limit_type.upper() == 'WAV':
            self.WAV1 = float(minval)
            self.WAV2 = float(maxval)
            self.DWAV = float(step_type + step_size)
        elif limit_type.upper() == 'Q':
            if not rebin_str is None:
                self.Q_REBIN = rebin_str
            else:
                self.Q_REBIN = minval + "," + step_type + step_size + "," + maxval
        elif limit_type.upper() == 'QXY':
            self.QXY2 = float(maxval)
            self.DQXY = float(step_type + step_size)
        elif limit_type.upper() == 'R':
            self.RMIN = float(minval)/1000.
            self.RMAX = float(maxval)/1000.
            self.DEF_RMIN = self.RMIN
            self.DEF_RMAX = self.RMAX
        elif limit_type.upper() == 'PHI':
            self.set_phi_limit(float(minval), float(maxval), True) 
        elif limit_type.upper() == 'PHI/NOMIRROR':
            self.set_phi_limit(float(minval), float(maxval), False)
        else:
            pass

    def _readMONValues(self, line):
        details = line[4:]
    
        #MON/LENTH, MON/SPECTRUM and MON/TRANS all accept the INTERPOLATE option
        interpolate = False
        interPlace = details.upper().find('/INTERPOLATE')
        if interPlace != -1 :
            interpolate = True
            details = details[0:interPlace]
    
        if details.upper().startswith('LENGTH'):
            self.suggest_monitor_spectrum(int(details.split()[1]), interpolate)
        
        elif details.upper().startswith('SPECTRUM'):
            self.set_monitor_spectrum(int(details.split('=')[1]), interpolate)
        
        elif details.upper().startswith('TRANS'):
            parts = details.split('=')
            if len(parts) < 2 or parts[0].upper() != 'TRANS/SPECTRUM' :
                _issueWarning('Unable to parse MON/TRANS line, needs MON/TRANS/SPECTRUM=')
            self.set_trans_spectrum(int(parts[1]), interpolate)        
    
        elif 'DIRECT' in details.upper():
            parts = details.split("=")
            if len(parts) == 2:
                filepath = parts[1].rstrip()
                if '[' in filepath:
                    idx = filepath.rfind(']')
                    filepath = filepath[idx + 1:]
                if not os.path.isabs(filepath):
                    filepath = os.path.join(self._user_path, filepath)
                type = parts[0]
                parts = type.split("/")
                if len(parts) == 1:
                    if parts[0].upper() == 'DIRECT':
                        self.DIRECT_BEAM_FILE_R = filepath
                        self.DIRECT_BEAM_FILE_F = filepath
                    elif parts[0].upper() == 'HAB':
                        self.DIRECT_BEAM_FILE_F = filepath
                    else:
                        pass
                elif len(parts) == 2:
                    detname = parts[1]
                    if detname.upper() == 'REAR':
                        self.DIRECT_BEAM_FILE_R = filepath
                    elif detname.upper() == 'FRONT' or detname.upper() == 'HAB':
                        self.DIRECT_BEAM_FILE_F = filepath
                    else:
                        _issueWarning('Incorrect detector specified for efficiency file "' + line + '"')
                else:
                    _issueWarning('Unable to parse monitor line "' + line + '"')
            else:
                _issueWarning('Unable to parse monitor line "' + line + '"')
                            
    def _readDetectorCorrections(self, details):
        values = details.split()
        det_name = values[0]
        det_axis = values[1]
        shift = float(values[2])
    
        if det_name == 'REAR':
            if det_axis == 'X':
                self.instrument.REAR_DET_X_CORR = shift
            elif det_axis == 'Z':
                self.instrument.REAR_DET_Z_CORR = shift
            else:
                pass
        else:
            if det_axis == 'X':
                self.instrument.FRONT_DET_X_CORR = shift
            elif det_axis == 'Y':
                self.instrument.FRONT_DET_Y_CORR = shift
            elif det_axis == 'Z':
                self.instrument.FRONT_DET_Z_CORR = shift
            elif det_axis == 'ROT':
                self.instrument.FRONT_DET_ROT_CORR = shift
            else:
                pass


    def set_workspace_name(self, name):
        RenameWorkspace(self._final_workspace, name)

    def _restore_defaults():
        Mask('MASK/CLEAR')
        Mask('MASK/CLEAR/TIME')
        SetRearEfficiencyFile(None)
        SetFrontEfficiencyFile(None)
        self.RMIN = self.RMAX = self.DEF_RMIN = self.DEF_RMAX
        self.WAV1 = self.WAV2 = self.DWAV = self.Q_REBIN = self.QXY = self.DQY = None
        global RESCALE, SAMPLE_GEOM, SAMPLE_WIDTH, SAMPLE_HEIGHT, SAMPLE_THICKNESS
        # Scaling values
        self.RESCALE = 100.  # percent
        INSTRUMENT.FRONT_DET_Z_CORR = INSTRUMENT.FRONT_DET_Y_CORR = INSTRUMENT.FRONT_DET_X_CORR = INSTRUMENT.FRONT_DET_ROT_CORR = 0.0
        INSTRUMENT.REAR_DET_Z_CORR = INSTRUMENT.REAR_DET_X_CORR = 0.0
        
        self.BACKMON_START = self.BACKMON_END = None
    
