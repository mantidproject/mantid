###########################################################################
# This is the template analysis script for reducing the SANS data.        #
# It has place holders so that the appropriate data can be filled in at   #
# run time by the SANS GUI                                                #
#                                                                         #
#  Authors: Russell Taylor, Tessella plc (Original script)                #
#           Martyn Gigg, Tessella plc (Adapted to GUI use)                #
#           with major input from Richard Heenan                          #
###########################################################################
#
#
# README: 
# This script contains a list of function definitions to perform the SANS data reduction.
# It was primarily designed as a back end to the GUI. When being used from the GUI, MantidPlot
# automatically replaces all of the information at the top of this module
#  

#
# Set up for cycle 09/02 for which the spectra are now by rows, right to left 
# from top right, and start at 9 after 8 monitors !
#
import SANSUtility
import math
from mantidsimple import *

# ---------------------------- CORRECTION INPUT -----------------------------------------
# The information between this line and the other '-----' delimiter needs to be provided
# for the script to function. From the GUI, the tags will be replaced by the appropriate
# information. 

# The tags get replaced by input from the GUI
# The workspaces
SCATTER_SAMPLE = None
SCATTER_CAN = ''
TRANS_SAMPLE = ''
TRANS_CAN = ''
DIRECT_SAMPLE = ''
DIRECT_CAN = ''
DIRECT_CAN = ''

# Now the mask string (can be empty)
# These apply to both detectors
SPECMASKSTRING = ''
TIMEMASKSTRING = ''
# These are for the separate detectors (R = main & F = HAB for LOQ)
SPECMASKSTRING_R = ''
SPECMASKSTRING_F = ''
TIMEMASKSTRING_R = ''
TIMEMASKSTRING_F = ''

# Instrument information
INSTR_DIR = mtd.getConfigProperty('instrumentDefinition.directory')
INSTR_NAME = 'SANS2D'
# Beam centre in metres
XBEAM_CENTRE = None
YBEAM_CENTRE = None

# Analysis tab values
RMIN = None
RMAX = None
DEF_RMIN = None
DEF_RMAX = None
WAV1 = None
WAV2 = None
DWAV = None
Q1 = None
Q2 = None
DQ = None
QXY2 = None
DQXY = None
DIRECT_BEAM_FILE_R = None
DIRECT_BEAM_FILE_F = None
GRAVITY = False
# This indicates whether a 1D or a 2D analysis is performed
CORRECTION_TYPE = '1D'
# Component positions
SAMPLE_Z_CORR = 0.0
PHIMIN=-90.0
PHIMAX=90.0

# Scaling values
RESCALE = 100.  # percent
SAMPLE_GEOM = 3
SAMPLE_WIDTH = 1.0
SAMPLE_HEIGHT = 1.0
SAMPLE_THICKNESS = 1.0 

# These values are used for the start and end bins for FlatBackground removal.
###############################################################################################
# RICHARD'S NOTE FOR SANS2D: these may need to vary with chopper phase and detector distance !
# !TASK! Put the values in the mask file if they need to be different ?????
##############################################################################################
# The GUI will replace these with default values of
# LOQ: 31000 -> 39000
# S2D: 85000 -> 100000
BACKMON_START = None
BACKMON_END = None

# The detector bank to look at. The GUI has an options box to select the detector to analyse. 
# The spectrum numbers are deduced from the name within the rear-detector tag. Names are from the 
# instrument definition file
# LOQ: HAB or main-detector-bank
# S2D: front-detector or rear-detector 
DETBANK = None

# The monitor spectrum taken from the GUI. Is this still necessary?? or can I just deduce
# it from the instrument name 
MONITORSPECTRUM = None

# Detector position information for SANS2D
FRONT_DET_RADIUS = 306.0
FRONT_DET_DEFAULT_SD_M = 4.0
FRONT_DET_DEFAULT_X_M = 1.1
REAR_DET_DEFAULT_SD_M = 4.0

# LOG files for SANS2D will have these encoder readings  
FRONT_DET_Z = 0.0
FRONT_DET_X = 0.0
FRONT_DET_ROT = 0.0
REAR_DET_Z = 0.0
# Rear_Det_X  Will Be Needed To Calc Relative X Translation Of Front Detector 
REAR_DET_X = 0.0

# MASK file stuff ==========================================================
# correction terms to SANS2d encoders - store in MASK file ?
FRONT_DET_Z_CORR = 0.0
FRONT_DET_Y_CORR = 0.0 
FRONT_DET_X_CORR = 0.0 
FRONT_DET_ROT_CORR = 0.0
REAR_DET_Z_CORR = 0.0 
REAR_DET_X_CORR = 0.0

#------------------------------- End of input section -----------------------------------------

# Transmission variables for SANS2D. The CalculateTransmission algorithm contains the defaults
# for LOQ so these are not used for LOQ
TRANS_WAV1 = 2.0
TRANS_WAV2 = 14.0
TRANS_UDET_MON = 2
TRANS_UDET_DET = 3

###################################################################################################################
#
#                              Interface functions (to be called from scripts or the GUI)
#
###################################################################################################################
_NOPRINT_ = False
_VERBOSE_ = False
# "Enumerations"
DefaultTrans = True
NewTrans = False
# Mismatched detectors
_MARKED_DETS_ = []

_DET_ABBREV = {'FRONT' : 'front-detector', 'REAR' : 'rear-detector', 'MAIN' : 'main-detector-bank', 'HAB' : 'HAB' }

def SetNoPrintMode(quiet = True):
    global _NOPRINT_
    _NOPRINT_ = quiet

def SetVerboseMode(state):
    global _VERBOSE_
    _VERBOSE_ = state

# Print a message and log it if the 
def _printMessage(msg, log = True):
    if log == True and _VERBOSE_ == True:
        mantid.sendLogMessage('::SANS::' + msg)
    if _NOPRINT_ == True: 
        return
    print msg

# Warn the user
def _issueWarning(msg):
    mantid.sendLogMessage('::SANS::Warning: ' + msg)
    if _NOPRINT_ == True:
        return
    print 'WARNING: ' + msg

# Fatal error
def _fatalError(msg):
    exit(msg)

DATA_PATH = ''
# Set the data directory
def DataPath(directory):
    _printMessage('DataPath("' + directory + '") - Will look for raw data here')
    if os.path.exists(directory) == False:
        _issueWarning("Data directory does not exist")
        return
    global DATA_PATH
    DATA_PATH = directory

USER_PATH = ''
# Set the user directory
def UserPath(directory):
    _printMessage('UserPath("' + directory + '") - Will look for mask file here')
    if os.path.exists(directory) == False:
        _issueWarning("Data directory does not exist")
        return
    global USER_PATH
    USER_PATH = directory

##################################################### 
# Access function for retrieving parameters
#####################################################
def printParameter(var):
    exec('print ' + var)

########################### 
# Instrument
########################### 
def SANS2D():
    _printMessage('SANS2D()')
    global INSTR_NAME, MONITORSPECTRUM
    INSTR_NAME = 'SANS2D'
    MONITORSPECTRUM = 2
    if DETBANK != 'rear-detector':
        Detector('rear-detector')

def LOQ():
    _printMessage('LOQ()')
    global INSTR_NAME, MONITORSPECTRUM
    INSTR_NAME = 'LOQ'
    MONITORSPECTRUM = 2
    if DETBANK != 'main-detector-bank':
        Detector('main-detector-bank')

def Detector(det_name):
    _printMessage('Detector("' + det_name + '")')
    # Deal with abbreviations
    lname = det_name.lower()
    if lname == 'front':
        det_name = 'front-detector'
    elif lname == 'rear':
        det_name = 'rear-detector'
    elif lname == 'main':
        det_name = 'main-detector-bank'
    elif lname == 'hab':
        det_name = 'HAB'
    else:
        pass
    global DETBANK

    if INSTR_NAME == 'SANS2D' and (det_name == 'rear-detector' or det_name == 'front-detector') or \
       INSTR_NAME == 'LOQ' and (det_name == 'main-detector-bank' or det_name == 'HAB'):
        DETBANK = det_name
    else:
        _issueWarning('Attempting to set invalid detector name "' + det_name + '" for instrument ' + INSTR_NAME)
        if INSTR_NAME == 'LOQ':
            _issueWarning('Setting default as main-detector-bank')
            DETBANK = 'main-detector-bank'
        else:
            _issueWarning('Setting default as rear-detector')
            DETBANK = 'rear-detector'

def Set1D():
    _printMessage('Set1D()')
    global CORRECTION_TYPE
    CORRECTION_TYPE = '1D'

def Set2D():
    _printMessage('Set2D()')
    global CORRECTION_TYPE
    CORRECTION_TYPE = '2D'

########################### 
# Set the scattering sample raw workspace
########################### 
_SAMPLE_SETUP = None
_SAMPLE_RUN = ''
def AssignSample(sample_run, reload = True):
    _printMessage('AssignSample("' + sample_run + '")')
    global SCATTER_SAMPLE, _SAMPLE_SETUP, _SAMPLE_RUN
    if( sample_run.startswith('.') or sample_run == '' or sample_run == None):
        _SAMPLE_SETUP = None
        _SAMPLE_RUN = ''
        SCATTER_SAMPLE = None
        return '', '()'
    
    _SAMPLE_RUN = sample_run
    SCATTER_SAMPLE,reset,logname,filepath = _assignHelper(sample_run, False, reload)
    if SCATTER_SAMPLE == '':
        _issueWarning('Unable to load sans sample run, cannot continue.')
        return '','()'
    if reset == True:
        _SAMPLE_SETUP = None
    if (INSTR_NAME == 'SANS2D'):
        global _MARKED_DETS_
        _MARKED_DETS_ = []
        logvalues = _loadDetectorLogs(logname,filepath)
        if logvalues == None:
            mtd.deleteWorkspace(SCATTER_SAMPLE)
            _issueWarning("Sample logs cannot be loaded, cannot continue")
            return '','()'
    else:
        return SCATTER_SAMPLE, None
        
    global FRONT_DET_Z, FRONT_DET_X, FRONT_DET_ROT, REAR_DET_Z, REAR_DET_X
    FRONT_DET_Z = float(logvalues['Front_Det_Z'])
    FRONT_DET_X = float(logvalues['Front_Det_X'])
    FRONT_DET_ROT = float(logvalues['Front_Det_Rot'])
    REAR_DET_Z = float(logvalues['Rear_Det_Z'])
    REAR_DET_X = float(logvalues['Rear_Det_X'])

    return SCATTER_SAMPLE, logvalues

########################### 
# Set the scattering can raw workspace
########################### 
_CAN_SETUP = None
_CAN_RUN = ''
def AssignCan(can_run, reload = True):
    _printMessage('AssignCan("' + can_run + '")')
    global SCATTER_CAN, _CAN_SETUP, _CAN_RUN
    if( can_run.startswith('.') or can_run == '' or can_run == None):
        SCATTER_CAN = ''
        _CAN_RUN = ''
        _CAN_SETUP = None
        return '', '()'

    _CAN_RUN = can_run
    SCATTER_CAN ,reset, logname,filepath = _assignHelper(can_run, False, reload)
    if SCATTER_CAN == '':
        _issueWarning('Unable to load sans can run, cannot continue.')
        return '','()'
    if reset == True:
        _CAN_SETUP  = None
    if (INSTR_NAME == 'SANS2D'):
        global _MARKED_DETS_
        _MARKED_DETS_ = []
        logvalues = _loadDetectorLogs(logname,filepath)
        if logvalues == None:
            _issueWarning("Can logs could not be loaded, using sample values.")
            return SCATTER_CAN, "()"
    else:
        return SCATTER_CAN, ""
    
    smp_values = []
    smp_values.append(FRONT_DET_Z + FRONT_DET_Z_CORR)
    smp_values.append(FRONT_DET_X + FRONT_DET_X_CORR)
    smp_values.append(FRONT_DET_ROT + FRONT_DET_ROT_CORR)
    smp_values.append(REAR_DET_Z + REAR_DET_Z_CORR)
    smp_values.append(REAR_DET_X + REAR_DET_X_CORR)

    # Check against sample values and warn if they are not the same but still continue reduction
    if len(logvalues) == 0:
        return  SCATTER_CAN, logvalues
    
    can_values = []
    can_values.append(float(logvalues['Front_Det_Z']) + FRONT_DET_Z_CORR)
    can_values.append(float(logvalues['Front_Det_X']) + FRONT_DET_X_CORR)
    can_values.append(float(logvalues['Front_Det_Rot']) + FRONT_DET_ROT_CORR)
    can_values.append(float(logvalues['Rear_Det_Z']) + REAR_DET_Z_CORR)
    can_values.append(float(logvalues['Rear_Det_X']) + REAR_DET_X_CORR)


    det_names = ['Front_Det_Z', 'Front_Det_X','Front_Det_Rot', 'Rear_Det_Z', 'Rear_Det_X']
    for i in range(0, 5):
        if math.fabs(smp_values[i] - can_values[i]) > 5e-03:
            _issueWarning(det_names[i] + " values differ between sample and can runs. Sample = " + str(smp_values[i]) + \
                              ' , Can = ' + str(can_values[i]))
            _MARKED_DETS_.append(det_names[i])
    
    return SCATTER_CAN, logvalues

########################### 
# Set the trans sample and measured raw workspaces
########################### 
def TransmissionSample(sample, direct, reload = True):
    _printMessage('TransmissionSample("' + sample + '","' + direct + '")')
    global TRANS_SAMPLE, DIRECT_SAMPLE
    TRANS_SAMPLE = _assignHelper(sample, True, reload)[0]
    DIRECT_SAMPLE = _assignHelper(direct, True, reload)[0]
    return TRANS_SAMPLE, DIRECT_SAMPLE

########################## 
# Set the trans sample and measured raw workspaces
########################## 
def TransmissionCan(can, direct, reload = True):
    _printMessage('TransmissionCan("' + can + '","' + direct + '")')
    global TRANS_CAN, DIRECT_CAN
    TRANS_CAN = _assignHelper(can, True, reload)[0]
    if direct == '' or direct == None:
        DIRECT_CAN = DIRECT_SAMPLE 
    else:
        DIRECT_CAN = _assignHelper(direct, True, reload)[0]
    return TRANS_CAN, DIRECT_CAN

# Helper function
def _assignHelper(run_string, is_trans, reload = True):
    if run_string == '' or run_string.startswith('.'):
        return '',True,'',''
    pieces = run_string.split('.')
    if len(pieces) != 2 :
         _fatalError("Invalid run specified: " + run_string + ". Please use RUNNUMBER.EXT format")
    else:
        run_no = pieces[0]
        ext = pieces[1]
    if run_no == '':
        return '',True,'',''
        
    if INSTR_NAME == 'LOQ':
        field_width = 5
    else:
        field_width = 8
        
    fullrun_no,logname,shortrun_no = padRunNumber(run_no, field_width)
    
    if is_trans:
        wkspname =  shortrun_no + '_trans_' + ext.lower()
    else:
        wkspname =  shortrun_no + '_sans_' + ext.lower()

    if reload == False and mtd.workspaceExists(wkspname):
        return wkspname,False,'',''

    basename = INSTR_NAME + fullrun_no
    filename = os.path.join(DATA_PATH,basename)
    # Workaround so that the FileProperty does the correct searching of data paths if this file doesn't exist
    if not os.path.exists(filename + '.' + ext):
        filename = basename
    
    if is_trans:
        try:
            if INSTR_NAME == 'SANS2D' and int(run_no) < 568:
                dimension = SANSUtility.GetInstrumentDetails(INSTR_NAME,DETBANK)[0]
                specmin = dimension*dimension*2
                specmax = specmin + 4
            else:
                specmin = None
                specmax = 8
                
            filepath = _loadRawData(filename, wkspname, ext, specmin,specmax)
        except RuntimeError:
            return '',True,'',''
    else:
        try:
            filepath = _loadRawData(filename, wkspname, ext)
        except RuntimeError:
            return '',True,'',''
    return wkspname,True, INSTR_NAME + logname, filepath

def padRunNumber(run_no, field_width):
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

##########################
# Loader function
##########################
def _loadRawData(filename, workspace, ext, spec_min = None, spec_max = None):
    if ext.lower().startswith('n'):
        alg = LoadNexus(filename + '.' + ext, workspace,SpectrumMin=spec_min,SpectrumMax=spec_max)
    else:
        alg = LoadRaw(filename + '.' + ext, workspace, SpectrumMin = spec_min,SpectrumMax = spec_max)
        LoadSampleDetailsFromRaw(workspace, filename + '.' + ext)

    sample_details = mtd.getMatrixWorkspace(workspace).getSampleDetails()
    SampleGeometry(sample_details.getGeometryFlag())
    SampleThickness(sample_details.getThickness())
    SampleHeight(sample_details.getHeight())
    SampleWidth(sample_details.getWidth())

    # Return the filepath actually used to load the data
    fullpath = alg.getPropertyValue("Filename")
    return os.path.dirname(fullpath)


# Load the detector logs
def _loadDetectorLogs(logname,filepath):
    # Adding runs produces a 1000nnnn or 2000nnnn. For less copying, of log files doctor the filename
    logname = logname[0:6] + '0' + logname[7:]
    filename = os.path.join(filepath, logname + '.log')

    # Build a dictionary of log data 
    logvalues = {}
    logvalues['Rear_Det_X'] = '0.0'
    logvalues['Rear_Det_Z'] = '0.0'
    logvalues['Front_Det_X'] = '0.0'
    logvalues['Front_Det_Z'] = '0.0'
    logvalues['Front_Det_Rot'] = '0.0'
    try:
        file_handle = open(filename, 'r')
    except IOError:
        _issueWarning("Log file \"" + filename + "\" could not be loaded.")
        return None
        
    for line in file_handle:
        parts = line.split()
        if len(parts) != 3:
            _issueWarning('Incorrect structure detected in logfile "' + filename + '" for line \n"' + line + '"\nEntry skipped')
        component = parts[1]
        if component in logvalues.keys():
            logvalues[component] = parts[2]
    
    file_handle.close()
    return logvalues

# Return the list of mismatched detector names
def GetMismatchedDetList():
    return _MARKED_DETS_

#########################
# Limits 
def LimitsR(rmin, rmax):
    _printMessage('LimitsR(' + str(rmin) + ',' +str(rmax) + ')')
    _readLimitValues('L/R ' + str(rmin) + ' ' + str(rmax) + ' 1')

def LimitsWav(lmin, lmax, step, type):
    _printMessage('LimitsWav(' + str(lmin) + ',' + str(lmax) + ',' + str(step) + ','  + type + ')')
    _readLimitValues('L/WAV ' + str(lmin) + ' ' + str(lmax) + ' ' + str(step) + '/'  + type)

def LimitsQ(qmin, qmax, step, type):
    _printMessage('LimitsQ(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(type))
    _readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type)

def LimitsQXY(qmin, qmax, step, type):
    _printMessage('LimitsQXY(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(type))
    _readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type)
    
def LimitsPhi(phimin, phimax):
    _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax))
    _readLimitValues('L/PHI ' + str(phimin) + ' ' + str(phimax))
	
def Gravity(flag):
    _printMessage('Gravity(' + str(flag) + ')')
    if isinstance(flag, bool) or isinstance(flag, int):
        global GRAVITY
        GRAVITY = flag
    else:
        _warnUser("Invalid GRAVITY flag passed, try True/False. Setting kept as " + str(GRAVITY))
        
###################################
# Scaling value
###################################
def _SetScales(scalefactor):
    global RESCALE
    RESCALE = scalefactor * 100.0

######################### 
# Sample geometry flag
######################### 
def SampleGeometry(geom_id):
    if geom_id > 3 or geom_id < 1:
        _issueWarning("Invalid geometry type for sample: " + str(geom_id) + ". Setting default to 3.")
        geom_id = 3
    global SAMPLE_GEOM
    SAMPLE_GEOM = geom_id

######################### 
# Sample width
######################### 
def SampleWidth(width):
    if SAMPLE_GEOM == None:
        _fatalError('Attempting to set width without setting geometry flag. Please set geometry type first')
    global SAMPLE_WIDTH
    SAMPLE_WIDTH = width
    # For a disk the height=width
    if SAMPLE_GEOM == 3:
        global SAMPLE_HEIGHT
        SAMPLE_HEIGHT = width

######################### 
# Sample height
######################### 
def SampleHeight(height):
    if SAMPLE_GEOM == None:
        _fatalError('Attempting to set height without setting geometry flag. Please set geometry type first')
    global SAMPLE_HEIGHT
    SAMPLE_HEIGHT = height
    # For a disk the height=width
    if SAMPLE_GEOM == 3:
        global SAMPLE_WIDTH
        SAMPLE_WIDTH = height

######################### 
# Sample thickness
#########################
def SampleThickness(thickness):
    global SAMPLE_THICKNESS
    SAMPLE_THICKNESS = thickness

#############################
# Print sample geometry
###########################
def displayGeometry():
    print 'Beam centre: [' + str(XBEAM_CENTRE) + ',' + str(YBEAM_CENTRE) + ']'
    print '-- Sample Geometry --\n' + \
        '    ID: ' + str(SAMPLE_GEOM) + '\n' + \
        '    Width: ' + str(SAMPLE_WIDTH) + '\n' + \
        '    Height: ' + str(SAMPLE_HEIGHT) + '\n' + \
        '    Thickness: ' + str(SAMPLE_THICKNESS) + '\n'


######################################
# Set the centre in mm
####################################
def SetCentre(XVAL, YVAL):
    _printMessage('SetCentre(' + str(XVAL) + ',' + str(YVAL) + ')')
    global XBEAM_CENTRE, YBEAM_CENTRE
    XBEAM_CENTRE = XVAL/1000.
    YBEAM_CENTRE = YVAL/1000.

#####################################
# Set the phi limit
#####################################
def SetPhiLimit(phimin,phimax):
    if phimin > phimax:
        phimin, phimax = phimax, phimin
    if abs(phimin) > 180.0:
        phimin = -90.0
    if abs(phimax) > 180.0:
        phimax = 90.0
	
    if phimax - phimin == 180.0:
        phimin = -90.0
        phimax = 90.0
    else:
        phimin = SANSUtility.normalizePhi(phimin)
        phimax = SANSUtility.normalizePhi(phimax)
  	
    global PHIMIN, PHIMAX
    PHIMIN = phimin
    PHIMAX = phimax

#####################################
# Clear current mask defaults
#####################################
def clearCurrentMaskDefaults():

    Mask('MASK/CLEAR')
    Mask('MASK/CLEAR/TIME')
    SetRearEfficiencyFile(None)
    SetFrontEfficiencyFile(None)
    global RMIN,RMAX, DEF_RMIN, DEF_RMAX
    RMIN = RMAX = DEF_RMIN = DEF_RMAX = None
    global WAV1, WAV2, DWAV, Q1, Q2, DQ, QXY2, DQY
    WAV1 = WAV2 = DWAV = Q1 = Q2 = DQ = QXY = DQY = None
    global SAMPLE_Z_CORR
    SAMPLE_Z_CORR = 0.0
    global RESCALE, SAMPLE_GEOM, SAMPLE_WIDTH, SAMPLE_HEIGHT, SAMPLE_THICKNESS
    # Scaling values
    RESCALE = 100.  # percent
    SAMPLE_GEOM = 3
    SAMPLE_WIDTH = SAMPLE_HEIGHT = SAMPLE_THICKNESS = 1.0
    global FRONT_DET_Z_CORR, FRONT_DET_Y_CORR, FRONT_DET_X_CORR, FRONT_DET_ROT_CORR 
    FRONT_DET_Z_CORR = FRONT_DET_Y_CORR = FRONT_DET_X_CORR = FRONT_DET_ROT_CORR = 0.0
    global REAR_DET_Z_CORR, REAR_DET_X_CORR
    REAR_DET_Z_CORR = REAR_DET_X_CORR = 0.0
    
    global BACKMON_START, BACKMON_END
    BACKMON_START = BACKMON_END = None

####################################
# Add a mask to the correct string
###################################
def Mask(details):
    _printMessage('Mask("' + details + '")')
    details = details.lstrip()
    details_compare = details.upper()
    if not details_compare.startswith('MASK'):
        _issueWarning('Ignoring malformed mask line ' + details)
        return

    global TIMEMASKSTRING, TIMEMASKSTRING_R, TIMEMASKSTRING_F,SPECMASKSTRING, SPECMASKSTRING_R, SPECMASKSTRING_F
    parts = details_compare.split('/')
    # A spectrum mask or mask range applied to both detectors
    if len(parts) == 1:
        spectra = details[4:].lstrip()
        if len(spectra.split()) == 1:
            SPECMASKSTRING += ',' + spectra
    elif len(parts) == 2:
        type = parts[1]
        detname = type.split()
        if type == 'CLEAR':
            SPECMASKSTRING = ''
            SPECMASKSTRING_R = ''
            SPECMASKSTRING_F = ''
        elif type.startswith('T'):
            if type.startswith('TIME'):
                bin_range = type[4:].lstrip()
            else:
                bin_range = type[1:].lstrip()
            TIMEMASKSTRING += ';' + bin_range
        elif len(detname) == 2:
            type = detname[0]
            if type in _DET_ABBREV.keys():
                spectra = detname[1]
                if type == 'FRONT' or type == 'HAB':
                    SPECMASKSTRING_F += ',' + spectra
                else:
                    SPECMASKSTRING_R += ',' + spectra
            else:
                _issueWarning('Unrecognized detector on mask line "' + details + '". Skipping line.')
        else:
            _issueWarning('Unrecognized masking option "' + details + '"')
    elif len(parts) == 3:
        type = parts[1]
        if type == 'CLEAR':
            TIMEMASKSTRING = ''
            TIMEMASKSTRING_R = ''
            TIMEMASKSTRING_F = ''
        elif (type == 'TIME' or type == 'T'):
            parts = parts[2].split()
            if len(parts) == 3:
                detname = parts[0].rstrip()
                bin_range = parts[1].rstrip() + ' ' + parts[2].lstrip() 
                if detname in _DET_ABBREV.keys():
                    if detname == 'FRONT' or detname == 'HAB':
                        TIMEMASKSTRING_F += ';' + bin_range
                    else:
                        TIMEMASKSTRING_R += ';' + bin_range
                else:
                    _issueWarning('Unrecognized detector on mask line "' + details + '". Skipping line.')
            else:
                _issueWarning('Unrecognized masking option "' + details + '"')
    else:
        pass

#############################
# Read a mask file
#############################
def MaskFile(filename):
    _printMessage('MaskFile("' + filename + '")')
    if os.path.isabs(filename) == False:
        filename = os.path.join(USER_PATH, filename)

    if os.path.exists(filename) == False:
        _fatalError("Cannot read mask file '" + filename + "', path does not exist.")
        
    clearCurrentMaskDefaults()

    file_handle = open(filename, 'r')
    for line in file_handle:
        if line.startswith('!'):
            continue
        # This is so that I can be sure all EOL characters have been removed
        line = line.lstrip().rstrip()
        upper_line = line.upper()
        if upper_line.startswith('L/'):
            _readLimitValues(line)
        elif upper_line.startswith('MON/'):
            details = line[4:]
            if details.upper().startswith('LENGTH'):
                SetMonitorSpectrum(int(details.split()[1]))
            elif 'DIRECT' in details.upper():
                parts = details.split("=")
                if len(parts) == 2:
                    filepath = parts[1].rstrip()
                    if '[' in filepath:
                        idx = filepath.rfind(']')
                        filepath = filepath[idx + 1:]
                    if not os.path.isabs(filepath):
                        filepath = os.path.join(USER_PATH, filepath)
                    type = parts[0]
                    parts = type.split("/")
                    if len(parts) == 1:
                        if parts[0] == 'DIRECT':
                            SetRearEfficiencyFile(filepath)
                            SetFrontEfficiencyFile(filepath)
                        elif parts[0] == 'HAB':
                            SetFrontEfficiencyFile(filepath)
                        else:
                            pass
                    elif len(parts) == 2:
                        detname = parts[1]
                        if detname == 'REAR':
                            SetRearEfficiencyFile(filepath)
                        elif detname == 'FRONT' or detname == 'HAB':
                            SetFrontEfficiencyFile(filepath)
                        else:
                            _issueWarning('Incorrect detector specified for efficiency file "' + line + '"')
                    else:
                        _issueWarning('Unable to parse monitor line "' + line + '"')
                else:
                    _issueWarning('Unable to parse monitor line "' + line + '"')
            else:
                continue
        elif upper_line.startswith('MASK'):
            Mask(upper_line)
        elif upper_line.startswith('SET CENTRE'):
            values = upper_line.split()
            SetCentre(float(values[2]), float(values[3]))
        elif upper_line.startswith('SET SCALES'):
            values = upper_line.split()
            _SetScales(float(values[2]))
        elif upper_line.startswith('SAMPLE/OFFSET'):
            values = upper_line.split()
            SetSampleOffset(values[1])
        elif upper_line.startswith('DET/'):
            type = upper_line[4:]
            if type.startswith('CORR'):
                _readDetectorCorrections(upper_line[8:])
            else:
                # This checks whether the type is correct and issues warnings if it is not
                Detector(type)
        elif upper_line.startswith('GRAVITY'):
            flag = upper_line[8:]
            if flag == 'ON':
                Gravity(True)
            elif flag == 'OFF':
                Gravity(False)
            else:
                _issueWarning("Gravity flag incorrectly specified, disabling gravity correction")
                Gravity(False)
        elif upper_line.startswith('BACK/MON/TIMES'):
            tokens = upper_line.split()
            global BACKMON_START, BACKMON_END
            if len(tokens) == 3:
                BACKMON_START = int(tokens[1])
                BACKMON_END = int(tokens[2])
            else:
                _issueWarning('Incorrectly formatted BACK/MON/TIMES line, not running FlatBackground.')
                BACKMON_START = None
                BACKMON_END = None

        else:
            continue

    # Close the handle
    file_handle.close()
    # Check if one of the efficency files hasn't been set and assume the other is to be used
    if DIRECT_BEAM_FILE_R == None and DIRECT_BEAM_FILE_F != None:
        SetRearEfficiencyFile(DIRECT_BEAM_FILE_F)
    if DIRECT_BEAM_FILE_F == None and DIRECT_BEAM_FILE_R != None:
        SetFrontEfficiencyFile(DIRECT_BEAM_FILE_R)

# Read a limit line of a mask file
def _readLimitValues(limit_line):
    limits = limit_line.partition('L/')[2]
    # Split with no arguments defaults to any whitespace character and in particular
    # multiple spaces are include
    elements = limits.split()
    type, minval, maxval = elements[0], elements[1], elements[2]
    if len(elements) == 4:
        step = elements[3]
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

    if type.upper() == 'WAV':
        global WAV1, WAV2, DWAV
        WAV1 = float(minval)
        WAV2 = float(maxval)
        DWAV = float(step_type + step_size)
    elif type.upper() == 'Q':
        global Q1, Q2, DQ
        Q1 = float(minval)
        Q2 = float(maxval)
        DQ = float(step_type + step_size)
    elif type.upper() == 'QXY':
        global QXY2, DQXY
        QXY2 = float(maxval)
        DQXY = float(step_type + step_size)
    elif type.upper() == 'R':
        global RMIN, RMAX, DEF_RMIN, DEF_RMAX
        RMIN = float(minval)/1000.
        RMAX = float(maxval)/1000.
        DEF_RMIN = RMIN
        DEF_RMAX = RMAX
    elif type.upper() == 'PHI':
    	SetPhiLimit(float(minval), float(maxval))
    else:
        pass

def _readDetectorCorrections(details):
    values = details.split()
    det_name = values[0]
    det_axis = values[1]
    shift = float(values[2])

    if det_name == 'REAR':
        if det_axis == 'X':
            global REAR_DET_X_CORR
            REAR_DET_X_CORR = shift
        elif det_axis == 'Z':
            global REAR_DET_Z_CORR
            REAR_DET_Z_CORR = shift
        else:
            pass
    else:
        if det_axis == 'X':
            global FRONT_DET_X_CORR
            FRONT_DET_X_CORR = shift
        elif det_axis == 'Y':
            global FRONT_DET_Y_CORR
            FRONT_DET_Y_CORR = shift
        elif det_axis == 'Z':
            global FRONT_DET_Z_CORR
            FRONT_DET_Z_CORR = shift
        elif det_axis == 'ROT':
            global FRONT_DET_ROT_CORR
            FRONT_DET_ROT_CORR = shift
        else:
            pass    

def SetSampleOffset(value):
    global SAMPLE_Z_CORR
    SAMPLE_Z_CORR = float(value)/1000.

def SetMonitorSpectrum(spec):
    global MONITORSPECTRUM
    MONITORSPECTRUM = spec
    
def SetRearEfficiencyFile(filename):
    global DIRECT_BEAM_FILE_R
    DIRECT_BEAM_FILE_R = filename
    
def SetFrontEfficiencyFile(filename):
    global DIRECT_BEAM_FILE_F
    DIRECT_BEAM_FILE_F = filename

def displayMaskFile():
    print '-- Mask file defaults --'
    print '    Wavelength range: ',WAV1, WAV2, DWAV
    print '    Q range: ',Q1, Q2, DQ
    print '    QXY range: ', QXY2, DQXY
    print '    radius', RMIN, RMAX
    print '    direct beam file rear:', DIRECT_BEAM_FILE_R
    print '    direct beam file front:', DIRECT_BEAM_FILE_F
    print '    global spectrum mask: ', SPECMASKSTRING
    print '    rear spectrum mask: ', SPECMASKSTRING_R
    print '    front spectrum mask: ', SPECMASKSTRING_F
    print '    global time mask: ', TIMEMASKSTRING
    print '    rear time mask: ', TIMEMASKSTRING_R
    print '    front time mask: ', TIMEMASKSTRING_F

# ---------------------------------------------------------------------------------------

##
# Set up the sample and can detectors and calculate the transmission if available
##
def _initReduction(xcentre = None, ycentre = None):
    # *** Sample setup first ***
    if SCATTER_SAMPLE == None:
        exit('Error: No sample run has been set')

    if xcentre == None or ycentre == None:
        xcentre = XBEAM_CENTRE
        ycentre = YBEAM_CENTRE

    global _SAMPLE_SETUP	
    if _SAMPLE_SETUP == None:
        _SAMPLE_SETUP = _init_run(SCATTER_SAMPLE, [xcentre, ycentre], False)
    
    global _CAN_SETUP
    if SCATTER_CAN != '' and _CAN_SETUP == None:
        _CAN_SETUP = _init_run(SCATTER_CAN, [xcentre, ycentre], True)

    # Instrument specific information using function in utility file
    global DIMENSION, SPECMIN, SPECMAX
    DIMENSION, SPECMIN, SPECMAX  = SANSUtility.GetInstrumentDetails(INSTR_NAME, DETBANK)

    return _SAMPLE_SETUP, _CAN_SETUP

##
# Run the reduction for a given wavelength range
##
def WavRangeReduction(wav_start = None, wav_end = None, use_def_trans = DefaultTrans, finding_centre = False):
    if wav_start == None:
        wav_start = WAV1
    if wav_end == None:
        wav_end = WAV2

    if finding_centre == False:
        _printMessage('WavRangeReduction(' + str(wav_start) + ',' + str(wav_end) + ',' + str(use_def_trans) + ',' + str(finding_centre) + ')')
        _printMessage("Running reduction for wavelength range " + str(wav_start) + '-' + str(wav_end))
    # This only performs the init if it needs to
    sample_setup, can_setup = _initReduction(XBEAM_CENTRE, YBEAM_CENTRE)

    wsname_cache = sample_setup.getReducedWorkspace()
    # Run correction function
    if finding_centre == True:
        final_workspace = wsname_cache.split('_')[0] + '_quadrants'
    else:
        final_workspace = wsname_cache + '_' + str(wav_start) + '_' + str(wav_end)
    sample_setup.setReducedWorkspace(final_workspace)
    # Perform correction
    Correct(sample_setup, wav_start, wav_end, use_def_trans, finding_centre)

    if can_setup != None:
        tmp_smp = final_workspace+"_sam_tmp"
        RenameWorkspace(final_workspace, tmp_smp)
        # Run correction function
        # was  Correct(SCATTER_CAN, can_setup[0], can_setup[1], wav_start, wav_end, can_setup[2], can_setup[3], finding_centre)
        tmp_can = final_workspace+"_can_tmp"
        can_setup.setReducedWorkspace(tmp_can)
        # Can correction
        Correct(can_setup, wav_start, wav_end, use_def_trans, finding_centre)
        Minus(tmp_smp, tmp_can, final_workspace)

        # Due to rounding errors, small shifts in detector encoders and poor stats in highest Q bins need "minus" the
        # workspaces before removing nan & trailing zeros thus, beware,  _sc,  _sam_tmp and _can_tmp may NOT have same Q bins
        if finding_centre == False:
            ReplaceSpecialValues(InputWorkspace = tmp_smp,OutputWorkspace = tmp_smp, NaNValue="0", InfinityValue="0")
            ReplaceSpecialValues(InputWorkspace = tmp_can,OutputWorkspace = tmp_can, NaNValue="0", InfinityValue="0")
            if CORRECTION_TYPE == '1D':
                SANSUtility.StripEndZeroes(tmp_smp)
                SANSUtility.StripEndZeroes(tmp_can)
        else:
            mantid.deleteWorkspace(tmp_smp)
            mantid.deleteWorkspace(tmp_can)
                
    # Crop Workspace to remove leading and trailing zeroes
    if finding_centre == False:
        # Replaces NANs with zeroes
        ReplaceSpecialValues(InputWorkspace = final_workspace, OutputWorkspace = final_workspace, NaNValue="0", InfinityValue="0")
        if CORRECTION_TYPE == '1D':
            SANSUtility.StripEndZeroes(final_workspace)
    else:
        RenameWorkspace(final_workspace + '_1', 'Left')
        RenameWorkspace(final_workspace + '_2', 'Right')
        RenameWorkspace(final_workspace + '_3', 'Up')
        RenameWorkspace(final_workspace + '_4', 'Down')
        UnGroupWorkspace(final_workspace)

    # Revert the name change so that future calls with different wavelengths get the correct name
    sample_setup.setReducedWorkspace(wsname_cache)                                
    return final_workspace

##
# Init helper
##
def _init_run(raw_ws, beamcoords, emptycell):
    if raw_ws == '':
        return None

    if emptycell:
        _printMessage('Initializing can workspace to [' + str(beamcoords[0]) + ',' + str(beamcoords[1]) + ']' )
    else:
        _printMessage('Initializing sample workspace to [' + str(beamcoords[0]) + ',' + str(beamcoords[1]) + ']' )

    if emptycell == True:
        final_ws = "can_temp_workspace"
    else:
        final_ws = raw_ws.split('_')[0]
        if DETBANK == 'front-detector':
            final_ws += 'front'
        elif DETBANK == 'rear-detector':
            final_ws += 'rear'
        elif DETBANK == 'main-detector-bank':
            final_ws += 'main'
        else:
            final_ws += 'HAB'
            final_ws += '_' + CORRECTION_TYPE

    # Put the components in the correct positions
    maskpt_rmin, maskpt_rmax = SetupComponentPositions(DETBANK, raw_ws, beamcoords[0], beamcoords[1])
    
    # Create a run details object
    if emptycell == True:
        return SANSUtility.RunDetails(raw_ws, final_ws, TRANS_CAN, DIRECT_CAN, maskpt_rmin, maskpt_rmax, 'can')
    else:
        return SANSUtility.RunDetails(raw_ws, final_ws, TRANS_SAMPLE, DIRECT_SAMPLE, maskpt_rmin, maskpt_rmax, 'sample')

##
# Setup the transmission workspace
##
def CalculateTransmissionCorrection(run_setup, lambdamin, lambdamax, use_def_trans):
    trans_raw = run_setup.getTransRaw()
    direct_raw = run_setup.getDirectRaw()
    if trans_raw == '' or direct_raw == '':
        return None

    if use_def_trans == DefaultTrans:
        if INSTR_NAME == 'SANS2D':
            fulltransws = trans_raw.split('_')[0] + '_trans_' + run_setup.getSuffix() + '_' + str(TRANS_WAV1) + '_' + str(TRANS_WAV2)
            wavbin = str(TRANS_WAV1) + ',' + str(DWAV) + ',' + str(TRANS_WAV2)
        else:
            fulltransws = trans_raw.split('_')[0] + '_trans_' + run_setup.getSuffix() + '_2.2_10'
            wavbin = str(2.2) + ',' + str(DWAV) + ',' + str(10.0)
    else:
        fulltransws = trans_raw.split('_')[0] + '_trans_' + run_setup.getSuffix() + '_' + str(lambdamin) + '_' + str(lambdamax)
        wavbin = str(lambdamin) + ',' + str(DWAV) + ',' + str(lambdamax)

    if mtd.workspaceExists(fulltransws) == False or use_def_trans == False:
        if INSTR_NAME == 'LOQ':
            # Change the instrument definition to the correct one in the LOQ case
            LoadInstrument(trans_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
            LoadInstrument(direct_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
            trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw, '1,2', BACKMON_START, BACKMON_END, wavbin, True)
            direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw, '1,2', BACKMON_START, BACKMON_END, wavbin, True)
            CalculateTransmission(trans_tmp_out,direct_tmp_out, fulltransws, OutputUnfittedData=True)
        else:
            trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw, '1,2', BACKMON_START, BACKMON_END, wavbin, False) 
            direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw, '1,2', BACKMON_START, BACKMON_END, wavbin, False)
            CalculateTransmission(trans_tmp_out,direct_tmp_out, fulltransws, TRANS_UDET_MON, TRANS_UDET_DET, TRANS_WAV1, TRANS_WAV2, OutputUnfittedData=True)
        # Remove temopraries
        mantid.deleteWorkspace(trans_tmp_out)
        mantid.deleteWorkspace(direct_tmp_out)

    if use_def_trans == DefaultTrans:
        tmp_ws = 'trans_' + run_setup.getSuffix() + '_' + str(lambdamin) + '_' + str(lambdamax)
        CropWorkspace(fulltransws, tmp_ws, XMin = str(lambdamin), XMax = str(lambdamax))
        return tmp_ws
    else: 
        return fulltransws

##
# Setup component positions, xbeam and ybeam in metres
##
def SetupComponentPositions(detector, dataws, xbeam, ybeam):
    # Put the components in the correct place
    # The sample holder
    MoveInstrumentComponent(dataws, 'some-sample-holder', Z = SAMPLE_Z_CORR, RelativePosition="1")
    
    # The detector
    if INSTR_NAME == 'LOQ':
        xshift = (317.5/1000.) - xbeam
        yshift = (317.5/1000.) - ybeam
        MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, RelativePosition="1")
        # LOQ instrument description has detector at 0.0, 0.0
        return [xshift, yshift], [xshift, yshift] 
    else:
        if detector == 'front-detector':
            rotateDet = (-FRONT_DET_ROT - FRONT_DET_ROT_CORR)
            RotateInstrumentComponent(dataws, detector,X="0.",Y="1.0",Z="0.",Angle=rotateDet)
            RotRadians = math.pi*(FRONT_DET_ROT + FRONT_DET_ROT_CORR)/180.
            xshift = (REAR_DET_X + REAR_DET_X_CORR - FRONT_DET_X - FRONT_DET_X_CORR + FRONT_DET_RADIUS*math.sin(RotRadians ) )/1000. - FRONT_DET_DEFAULT_X_M - xbeam
            yshift = (FRONT_DET_Y_CORR /1000.  - ybeam)
            # default in instrument description is 23.281m - 4.000m from sample at 19,281m !
            # need to add ~58mm to det1 to get to centre of detector, before it is rotated.
            zshift = (FRONT_DET_Z + FRONT_DET_Z_CORR + FRONT_DET_RADIUS*(1 - math.cos(RotRadians)) )/1000. - FRONT_DET_DEFAULT_SD_M
            MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0, 0.0], [0.0, 0.0]
        else:
            xshift = -xbeam
            yshift = -ybeam
            zshift = (REAR_DET_Z + REAR_DET_Z_CORR)/1000. - REAR_DET_DEFAULT_SD_M
            mantid.sendLogMessage("::SANS:: Setup move "+str(xshift*1000.)+" "+str(yshift*1000.))
            MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
            return [0.0,0.0], [xshift, yshift]

##
# Apply the spectrum and time masks to a given workspace
##
def applyMasking(workspace, firstspec, dimension, orientation=SANSUtility.Orientation.Horizontal,limitphi = True):
    # Spectra masks
    speclist = SANSUtility.ConvertToSpecList(SPECMASKSTRING, firstspec, dimension,orientation)
    _printMessage(str(speclist))
    SANSUtility.MaskBySpecNumber(workspace, speclist)

    #Time mask
    SANSUtility.MaskByBinRange(workspace,TIMEMASKSTRING)
    # Front detector
    if DETBANK == 'front-detector' or DETBANK == 'HAB':
        specstring = SPECMASKSTRING_F
        timestring = TIMEMASKSTRING_F
    else:
        specstring = SPECMASKSTRING_R
        timestring = TIMEMASKSTRING_R

    speclist = SANSUtility.ConvertToSpecList(specstring, firstspec, dimension,orientation) 
    SANSUtility.MaskBySpecNumber(workspace, speclist)
    SANSUtility.MaskByBinRange(workspace, timestring)
    
    # Finally apply masking to get the correct phi range
    if (limitphi == True and PHIMAX - PHIMIN < 180):
        centre = mtd[workspace].getInstrument().getComponentByName(DETBANK).getPos()
        centre_pt = [centre.getX(), centre.getY(), centre.getZ()]
        SANSUtility.LimitPhi(workspace, centre_pt, PHIMIN,PHIMAX)
        
#----------------------------------------------------------------------------------------------------------------------------
##
# Main correction routine
##
def Correct(run_setup, wav_start, wav_end, use_def_trans, finding_centre = False):
    '''Performs the data reduction steps'''
    global SPECMIN, SPECMAX, MONITORSPECTRUM
    sample_raw = run_setup.getRawWorkspace()
    orientation = orientation=SANSUtility.Orientation.Horizontal
    if INSTR_NAME == "SANS2D":
        sample_run = sample_raw.split('_')[0]
        ndigits = len(sample_run)
        if ndigits > 4:
            base_runno = int(sample_run[ndigits-4:])
        else:
            base_runno = int(sample_run)
        if base_runno < 568:
            MONITORSPECTRUM = 73730
            orientation=SANSUtility.Orientation.Vertical
            if DETBANK == 'front-detector':
                SPECMIN = DIMENSION*DIMENSION + 1 
                SPECMAX = DIMENSION*DIMENSION*2
            else:
                SPECMIN = 1
                SPECMAX = DIMENSION*DIMENSION
        elif (base_runno >= 568 and base_runno < 684):
            orientation = SANSUtility.Orientation.Rotated
        else:
            pass

    ############################# Setup workspaces ######################################
    monitorWS = "Monitor"
    _printMessage('monitor ' + str(MONITORSPECTRUM), True)
    # Get the monitor ( StartWorkspaceIndex is off by one with cropworkspace)
    CropWorkspace(sample_raw, monitorWS, StartWorkspaceIndex = str(MONITORSPECTRUM - 1), EndWorkspaceIndex = str(MONITORSPECTRUM - 1))
    if INSTR_NAME == 'LOQ':
        RemoveBins(monitorWS, monitorWS, '19900', '20500', Interpolation="Linear")
    
    # Remove flat background
    if BACKMON_START != None and BACKMON_END != None:
        FlatBackground(monitorWS, monitorWS, StartX = BACKMON_START, EndX = BACKMON_END, WorkspaceIndexList = '0')
    
    # Get the bank we are looking at
    final_result = run_setup.getReducedWorkspace()
    CropWorkspace(sample_raw, final_result, StartWorkspaceIndex = (SPECMIN - 1), EndWorkspaceIndex = str(SPECMAX - 1))
    #####################################################################################
        
    ########################## Masking  ################################################
    # Mask the corners and beam stop if radius parameters are given
    maskpt_rmin = run_setup.getMaskPtMin()
    maskpt_rmax = run_setup.getMaskPtMax()
    if finding_centre == True:
        if RMIN > 0.0: 
            SANSUtility.MaskInsideCylinder(final_result, RMIN, maskpt_rmin[0], maskpt_rmin[1])
        if RMAX > 0.0:
            SANSUtility.MaskOutsideCylinder(final_result, RMAX, maskpt_rmin[0], maskpt_rmin[1])
    else:
        if RMIN > 0.0: 
            SANSUtility.MaskInsideCylinder(final_result, RMIN, maskpt_rmin[0], maskpt_rmin[1])
        if RMAX > 0.0:
            SANSUtility.MaskOutsideCylinder(final_result, RMAX, maskpt_rmax[0], maskpt_rmax[1])

    applyMasking(final_result, SPECMIN, DIMENSION, orientation,True)
    ####################################################################################
        
    ######################## Unit change and rebin #####################################
    # Convert all of the files to wavelength and rebin
    # ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
    ConvertUnits(monitorWS, monitorWS, "Wavelength")
    wavbin =  str(wav_start) + "," + str(DWAV) + "," + str(wav_end)
    Rebin(monitorWS, monitorWS,wavbin)
    ConvertUnits(final_result,final_result,"Wavelength")
    Rebin(final_result,final_result,wavbin)
    ####################################################################################

    ####################### Correct by incident beam monitor ###########################
    # At this point need to fork off workspace name to keep a workspace containing raw counts
    tmpWS = "reduce_temp_workspace"
    Divide(final_result, monitorWS, tmpWS)
    mantid.deleteWorkspace(monitorWS)
    ###################################################################################

    ############################ Transmission correction ##############################
    trans_ws = CalculateTransmissionCorrection(run_setup, wav_start, wav_end, use_def_trans)
    if trans_ws != None:
        Divide(tmpWS, trans_ws, tmpWS)
    ##################################################################################   
        
    ############################ Efficiency correction ################################
    if DETBANK == 'rear-detector' or 'main-detector-bank':
        CorrectToFile(tmpWS, DIRECT_BEAM_FILE_R, tmpWS, "Wavelength", "Divide")
    else:
        CorrectToFile(tmpWS, DIRECT_BEAM_FILE_F, tmpWS, "Wavelength", "Divide")
    ###################################################################################
        
    ############################# Scale by volume #####################################
    SANSUtility.ScaleByVolume(tmpWS, RESCALE, SAMPLE_GEOM, SAMPLE_WIDTH, SAMPLE_HEIGHT, SAMPLE_THICKNESS)
    ################################################## ################################
        
    ################################ Correction in Q space ############################
    # 1D
    if CORRECTION_TYPE == '1D':
        q_bins = str(Q1) + "," + str(DQ) + "," + str(Q2)
        if finding_centre == True:
            GroupIntoQuadrants(tmpWS, final_result, maskpt_rmin[0], maskpt_rmin[1], q_bins)
            return
        else:
            Q1D(tmpWS,final_result,final_result,q_bins, AccountForGravity=GRAVITY)
    # 2D    
    else:
        # Run 2D algorithm
        Qxy(tmpWS, final_result, QXY2, DQXY)

    mantid.deleteWorkspace(tmpWS)
    return
############################# End of Correct function ###################################################

############################ Centre finding functions ###################################################

# These variables keep track of the centre coordinates that have been used so that we can calculate a relative shift of the
# detector
XVAR_PREV = 0.0
YVAR_PREV = 0.0
ITER_NUM = 0
RESIDUE_GRAPH = None
                
# Create a workspace with a quadrant value in it 
def CreateQuadrant(reduced_ws, rawcount_ws, quadrant, xcentre, ycentre, q_bins, output):
    # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
    CloneWorkspace(reduced_ws,output)
    objxml = SANSUtility.QuadrantXML([xcentre, ycentre, 0.0], RMIN, RMAX, quadrant)
    # Mask out everything outside the quadrant of interest
    MaskDetectorsInShape(output,objxml)
    # Q1D ignores masked spectra/detectors. This is on the InputWorkspace, so we don't need masking of the InputForErrors workspace
    Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)

    flag_value = -10.0
    ReplaceSpecialValues(InputWorkspace=output,OutputWorkspace=output,NaNValue=flag_value,InfinityValue=flag_value)
    if CORRECTION_TYPE == '1D':
        SANSUtility.StripEndZeroes(output, flag_value)

# Create 4 quadrants for the centre finding algorithm and return their names
def GroupIntoQuadrants(reduced_ws, final_result, xcentre, ycentre, q_bins):
    tmp = 'quad_temp_holder'
    pieces = ['Left', 'Right', 'Up', 'Down']
    to_group = ''
    counter = 0
    for q in pieces:
        counter += 1
        to_group += final_result + '_' + str(counter) + ','
        CreateQuadrant(reduced_ws, final_result, q, xcentre, ycentre, q_bins, final_result + '_' + str(counter))

    # We don't need these now 
    mantid.deleteWorkspace(final_result)                    
    mantid.deleteWorkspace(reduced_ws)
    GroupWorkspaces(final_result, to_group.strip(','))

# Calcluate the sum squared difference of the given workspaces. This assumes that a workspace with
# one spectrum for each of the quadrants. The order should be L,R,U,D.
def CalculateResidue():
    global XVAR_PREV, YVAR_PREV, RESIDUE_GRAPH
    yvalsA = mtd.getMatrixWorkspace('Left').readY(0)
    yvalsB = mtd.getMatrixWorkspace('Right').readY(0)
    qvalsA = mtd.getMatrixWorkspace('Left').readX(0)
    qvalsB = mtd.getMatrixWorkspace('Right').readX(0)
    qrange = [len(yvalsA), len(yvalsB)]
    nvals = min(qrange)
    residueX = 0
    indexB = 0
    for indexA in range(0, nvals):
        if qvalsA[indexA] < qvalsB[indexB]:
            mantid.sendLogMessage("::SANS::LR1 "+str(indexA)+" "+str(indexB))
            continue
        elif qvalsA[indexA] > qvalsB[indexB]:
            while qvalsA[indexA] > qvalsB[indexB]:
                mantid.sendLogMessage("::SANS::LR2 "+str(indexA)+" "+str(indexB))
                indexB += 1
        if indexA > nvals - 1 or indexB > nvals - 1:
            break
        residueX += pow(yvalsA[indexA] - yvalsB[indexB], 2)
        indexB += 1

    yvalsA = mtd.getMatrixWorkspace('Up').readY(0)
    yvalsB = mtd.getMatrixWorkspace('Down').readY(0)
    qvalsA = mtd.getMatrixWorkspace('Up').readX(0)
    qvalsB = mtd.getMatrixWorkspace('Down').readX(0)
    qrange = [len(yvalsA), len(yvalsB)]
    nvals = min(qrange)
    residueY = 0
    indexB = 0
    for indexA in range(0, nvals):
        if qvalsA[indexA] < qvalsB[indexB]:
            mantid.sendLogMessage("::SANS::UD1 "+str(indexA)+" "+str(indexB))
            continue
        elif qvalsA[indexA] > qvalsB[indexB]:
            while qvalsA[indexA] > qvalsB[indexB]:
                mantid.sendLogMessage("::SANS::UD2 "+str(indexA)+" "+str(indexB))
                indexB += 1
        if indexA > nvals - 1 or indexB > nvals - 1:
            break
        residueY += pow(yvalsA[indexA] - yvalsB[indexB], 2)
        indexB += 1
                        
    if RESIDUE_GRAPH == None:
        RESIDUE_GRAPH = plotSpectrum('Left', 0)
        mergePlots(RESIDUE_GRAPH, plotSpectrum('Right', 0))
        mergePlots(RESIDUE_GRAPH, plotSpectrum('Up', 0))
        mergePlots(RESIDUE_GRAPH, plotSpectrum('Down', 0))
	
    RESIDUE_GRAPH.activeLayer().setTitle("Itr " + str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))
    mantid.sendLogMessage("::SANS::Itr: "+str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))              
    return residueX, residueY
	
def RunReduction(coords):
    '''Compute the value of (L-R)^2+(U-D)^2 a circle split into four quadrants'''
    global XVAR_PREV, YVAR_PREV, RESIDUE_GRAPH
    xcentre = coords[0]
    ycentre= coords[1]
    
    xshift = -xcentre + XVAR_PREV
    yshift = -ycentre + YVAR_PREV
    XVAR_PREV = xcentre
    YVAR_PREV = ycentre

    # Do the correction
    if xshift != 0.0 or yshift != 0.0:
        MoveInstrumentComponent(SCATTER_SAMPLE, ComponentName = DETBANK, X = str(xshift), Y = str(yshift), RelativePosition="1")
        if SCATTER_CAN != '':
            MoveInstrumentComponent(SCATTER_CAN, ComponentName = DETBANK, X = str(xshift), Y = str(yshift), RelativePosition="1")
			
    _SAMPLE_SETUP.setMaskPtMin([0.0,0.0])
    _SAMPLE_SETUP.setMaskPtMax([xcentre, ycentre])
    if _CAN_SETUP != None:
        _CAN_SETUP.setMaskPtMin([0.0, 0.0])
        _CAN_SETUP.setMaskPtMax([xcentre, ycentre])

    WavRangeReduction(WAV1, WAV2, DefaultTrans, finding_centre = True)
    return CalculateResidue()

def FindBeamCentre(rlow, rupp, MaxIter = 10, xstart = None, ystart = None):
    global XVAR_PREV, YVAR_PREV, ITER_NUM, RMIN, RMAX, XBEAM_CENTRE, YBEAM_CENTRE
    RMIN = float(rlow)/1000.
    RMAX = float(rupp)/1000.
	
    if xstart == None or ystart == None:
        XVAR_PREV = XBEAM_CENTRE
        YVAR_PREV = YBEAM_CENTRE
    else:
        XVAR_PREV = xstart
        YVAR_PREV = ystart

    mantid.sendLogMessage("::SANS:: xstart,ystart="+str(XVAR_PREV*1000.)+" "+str(YVAR_PREV*1000.)) 
    _printMessage("Starting centre finding routine ...")
    # Initialize the workspace with the starting coordinates. (Note that this moves the detector to -x,-y)
    _initReduction(XVAR_PREV, YVAR_PREV)
    ITER_NUM = 0
    # Run reduction, returning the X and Y sum-squared difference values 
    _printMessage("Running initial reduction: " + str(XVAR_PREV*1000.)+ "  "+ str(YVAR_PREV*1000.))
    oldX2,oldY2 = RunReduction([XVAR_PREV, YVAR_PREV])
    XSTEP = 5.0/1000.
    YSTEP = 5.0/1000.
    # take first trial step
    XNEW = XVAR_PREV + XSTEP
    YNEW = YVAR_PREV + YSTEP
    for ITER_NUM in range(1, MaxIter+1):
        _printMessage("Iteration " + str(ITER_NUM) + ": " + str(XNEW*1000.)+ "  "+ str(YNEW*1000.))
        newX2,newY2 = RunReduction([XNEW, YNEW])
        if newX2 > oldX2:
            XSTEP = -XSTEP/2.
        if newY2 > oldY2:
            YSTEP = -YSTEP/2.
        if abs(XSTEP) < 0.1251/1000. and abs(YSTEP) < 0.1251/1000. :
            _printMessage("::SANS:: Converged - check if stuck in local minimum!")
            break
        oldX2 = newX2
        oldY2 = newY2
        XNEW += XSTEP
        YNEW += YSTEP
	
    if ITER_NUM == MaxIter:
        _printMessage("::SANS:: Out of iterations, new coordinates may not be the best!")
        XNEW -= XSTEP
        YNEW -= YSTEP

    
    XBEAM_CENTRE = XNEW
    YBEAM_CENTRE = YNEW
    _printMessage("Centre coordinates updated: [" + str(XBEAM_CENTRE*1000.)+ ","+ str(YBEAM_CENTRE*1000.) + ']')
    
    # Reload the sample and can and reset the radius range
    global _SAMPLE_SETUP
    _assignHelper(_SAMPLE_RUN, False)
    _SAMPLE_SETUP = None
    if _CAN_RUN != '':
        _assignHelper(_CAN_RUN, False)
        global _CAN_SETUP
        _CAN_SETUP = None
    
    RMIN = DEF_RMIN
    RMAX = DEF_RMAX

##
# Plot the results on the correct type of plot
##
def PlotResult(workspace):
    if CORRECTION_TYPE == '1D':
        plotSpectrum(workspace,0)
    else:
        qti.app.mantidUI.importMatrixWorkspace(workspace).plotGraph2D()

##################### View mask details #####################################################

def ViewCurrentMask():
    top_layer = 'CurrentMask'
    LoadEmptyInstrument(INSTR_DIR + '/' + INSTR_NAME + "_Definition.xml",top_layer)
    if RMIN > 0.0: 
        SANSUtility.MaskInsideCylinder(top_layer, RMIN, XBEAM_CENTRE, YBEAM_CENTRE)
    if RMAX > 0.0:
        SANSUtility.MaskOutsideCylinder(top_layer, RMAX, 0.0, 0.0)
    
    if INSTR_NAME == "SANS2D":
        firstspec = 5
    else:
        firstspec = 3

    dimension = SANSUtility.GetInstrumentDetails(INSTR_NAME, DETBANK)[0]
    applyMasking(top_layer, firstspec, dimension, SANSUtility.Orientation.HorizontalFlipped, False)
    
    # Mark up "dead" detectors with error value 
    FindDeadDetectors(top_layer, top_layer, DeadValue=500)

    # Visualise the result
    instrument_win = qti.app.mantidUI.getInstrumentView(top_layer)
    instrument_win.showWindow()

############################################################################################################################

############################################################################################
# Print a test script for Colette if asked
def createColetteScript(inputdata, format, reduced, centreit , plotresults, csvfile = '', savepath = ''):
    script = ''
    if csvfile != '':
        script += '[COLETTE]  @ ' + csvfile + '\n'
    file_1 = inputdata['sample_sans'] + format
    script += '[COLETTE]  ASSIGN/SAMPLE ' + file_1 + '\n'
    file_1 = inputdata['sample_trans'] + format
    file_2 = inputdata['sample_direct_beam'] + format
    script += '[COLETTE]  TRANSMISSION/SAMPLE/MEASURED ' + file_1 + ' ' + file_2 + '\n'
    file_1 = inputdata['can_sans'] + format
    script +='[COLETTE]  ASSIGN/CAN ' + file_1 + '\n'
    file_1 = inputdata['can_trans'] + format
    file_2 = inputdata['can_direct_beam'] + format
    script += '[COLETTE]  TRANSMISSION/CAN/MEASURED ' + file_1 + ' ' + file_2 + '\n'
    if centreit:
        script += '[COLETTE]  FIT/MIDDLE'
    # Parameters
    script += '[COLETTE]  LIMIT/RADIUS ' + str(RMIN) + ' ' + str(RMAX) + '\n'
    script += '[COLETTE]  LIMIT/WAVELENGTH ' + str(WAV1) + ' ' + str(WAV2) + '\n'
    if DWAV <  0:
        script += '[COLETTE]  STEP/WAVELENGTH/LOGARITHMIC ' + str(DWAV)[1:] + '\n'
    else:
        script += '[COLETTE]  STEP/WAVELENGTH/LINEAR ' + str(DWAV) + '\n'
    if CORRECTION_TYPE == '1D':
        script += '[COLETTE]  LIMIT/Q ' + str(Q1) + ' ' + str(Q2) + '\n'
        if DQ <  0:
            script += '[COLETTE]  STEP/Q/LOGARITHMIC ' + str(DQ)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/Q/LINEAR ' + str(DQ) + '\n'
    else:
        script += '[COLETTE]  LIMIT/QXY ' + str(0.0) + ' ' + str(QXY2) + '\n'
        if DQXY <  0:
            script += '[COLETTE]  STEP/QXY/LOGARITHMIC ' + str(DQXY)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/QXY/LINEAR ' + str(DQXY) + '\n'
    
    # Correct
    script += '[COLETTE] CORRECT'
    if plotresults:
        script += '[COLETTE]  DISPLAY/HISTOGRAM ' + reduced
    if savepath != '':
        script += '[COLETTE]  WRITE/LOQ ' + reduced + ' ' + savepath
        
    return script

############################################################################################

# These are to work around for the moment
def plotSpectrum(name, spec):
    return qti.app.mantidUI.pyPlotSpectraList([name],[spec])

def mergePlots(g1, g2):
    return qti.app.mantidUI.mergePlots(g1,g2)
