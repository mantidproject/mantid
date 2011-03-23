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
import isis_instrument as SANSInsts
import math
from mantidsimple import *
##START REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)
# disable plotting if running outside Mantidplot
try:
    from mantidplot import plotSpectrum, mergePlots
except ImportError:
    def plotSpectrum(a,b):
        pass
    def mergePlots(a,b):
        pass

try:
    from PyQt4.QtGui import qApp
    def appwidgets():
        return qApp.allWidgets()
except ImportError:
    def appwidgets():
        return []
##END REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)

NORMALISATION_FILE = None
# The workspaces
SCATTER_SAMPLE = None
SCATTER_CAN = SANSUtility.WorkspaceDetails('', -1)
TRANS_SAMPLE = ''
TRANS_CAN = ''
PERIOD_NOS = { "SCATTER_SAMPLE":1, "SCATTER_CAN":1 }
DIRECT_SAMPLE = ''
DIRECT_CAN = ''
DIRECT_CAN = ''
# if the workspaces come from multi-period i.e. group workspaces, the number of periods in that group will be stored in the following variables. These variables corrospond with those above
_SAMPLE_N_PERIODS = -1
_CAN_N_PERIODS =-1
_TRANS_SAMPLE_N_PERIODS = -1
DIRECT_SAMPLE_N_PERIODS = -1
TRANS_SAMPLE_N_CAN = -1
DIRECT_SAMPLE_N_CAN = -1

#This is stored as UserFile in the output workspace
MASKFILE = '_ no file'
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

# Instrument object. Start with a default
#TODO: get rid of all those globals
INSTRUMENT = SANSInsts.SANS2D()

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
Q_REBIN = None
QXY2 = None
DQXY = None
DIRECT_BEAM_FILE_R = None
DIRECT_BEAM_FILE_F = None
GRAVITY = False
# This indicates whether a 1D or a 2D analysis is performed
CORRECTION_TYPE = '1D'
# Component positions
PHIMIN=-90.0
PHIMAX=90.0
PHIMIRROR=True

# Scaling values
RESCALE = 100.  # percent
SAMPLE_GEOM = 3
SAMPLE_WIDTH = 1.0
SAMPLE_HEIGHT = 1.0
SAMPLE_THICKNESS = 1.0 

# These values are used for the start and end bins for FlatBackground removal.
BACKMON_START = None
BACKMON_END = None

# Transmission variables
TRANS_FIT_DEF = 'Log'
TRANS_FIT = TRANS_FIT_DEF
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
TRANS_WAV1 = None
TRANS_WAV2 = None
TRANS_WAV1_FULL = None
TRANS_WAV2_FULL = None

###################################################################################################################
#
#                              Interface functions (to be called from scripts or the GUI)
#
###################################################################################################################
DefaultTrans = True
NewTrans = False
##START REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)
_NOPRINT_ = False
_VERBOSE_ = False
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
##END REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)
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
    _printMessage('UserPath("' + directory + '") #Will look for mask file here')
    if os.path.exists(directory) == False:
        _issueWarning("Data directory does not exist")
        return
    global USER_PATH
    USER_PATH = directory

##################################################### 
# Access function for retrieving parameters
#####################################################
def printParameter(var):
    try:
        exec('print ' + var + ',')
    except:
        _issueWarning("Could not find parameter " + var + ", trying instrument object")
        exec('print INSTRUMENT.' + var)

########################### 
# Instrument
########################### 
def SANS2D():
    _printMessage('SANS2D()')
    global INSTRUMENT, TRANS_WAV1, TRANS_WAV2, TRANS_WAV1_FULL, TRANS_WAV2_FULL
    INSTRUMENT = SANSInsts.SANS2D()

    TRANS_WAV1_FULL = TRANS_WAV1 = 2.0
    TRANS_WAV2_FULL = TRANS_WAV2 = 14.0
    INSTRUMENT.lowAngDetSet = True

def LOQ():
    _printMessage('LOQ()')
    global INSTRUMENT, TRANS_WAV1, TRANS_WAV2, TRANS_WAV1_FULL, TRANS_WAV2_FULL
    INSTRUMENT = SANSInsts.LOQ()

    TRANS_WAV1_FULL = TRANS_WAV1 = 2.2
    TRANS_WAV2_FULL = TRANS_WAV2 = 10.0
    INSTRUMENT.lowAngDetSet = True

def Detector(det_name):
    _printMessage('Detector("' + det_name + '")')
    global INSTRUMENT
    if not INSTRUMENT.setDetector(det_name) :
        _issueWarning('Detector not found')
        _issueWarning('Detector set to ' + INSTRUMENT.cur_detector().name() + ' in ' + INSTRUMENT.name())

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
def AssignSample(sample_run, reload = True, period = -1):
    _printMessage('AssignSample("' + sample_run + '")')
    global SCATTER_SAMPLE, _SAMPLE_SETUP, _SAMPLE_RUN, _SAMPLE_N_PERIODS, PERIOD_NOS
    
    __clearPrevious(SCATTER_SAMPLE,others=[SCATTER_CAN,TRANS_SAMPLE,TRANS_CAN,DIRECT_SAMPLE,DIRECT_CAN])
    _SAMPLE_N_PERIODS = -1
    
    if( sample_run.startswith('.') or sample_run == '' or sample_run == None):
        _SAMPLE_SETUP = None
        _SAMPLE_RUN = ''
        SCATTER_SAMPLE = None
        return '', '()'
    
    _SAMPLE_RUN = sample_run
    SCATTER_SAMPLE,reset,logname,filepath, _SAMPLE_N_PERIODS = _assignHelper(sample_run, False, reload, period)
    if SCATTER_SAMPLE.getName() == '':
        _issueWarning('Unable to load SANS sample run, cannot continue.')
        return '','()'
    if reset == True:
        _SAMPLE_SETUP = None

    try:
        logvalues = INSTRUMENT.load_detector_logs(logname,filepath)
        if logvalues == None:
            mtd.deleteWorkspace(SCATTER_SAMPLE.getName())
            _issueWarning("Sample logs cannot be loaded, cannot continue")
            return '','()'
    except AttributeError:
        if not INSTRUMENT.name() == 'LOQ': raise
    
    PERIOD_NOS["SCATTER_SAMPLE"] = period

    if (INSTRUMENT.name() == 'LOQ'):
        return SCATTER_SAMPLE.getName(), None

    #global FRONT_DET_Z, FRONT_DET_X, FRONT_DET_ROT, REAR_DET_Z, REAR_DET_X
    INSTRUMENT.FRONT_DET_Z = float(logvalues['Front_Det_Z'])
    INSTRUMENT.FRONT_DET_X = float(logvalues['Front_Det_X'])
    INSTRUMENT.FRONT_DET_ROT = float(logvalues['Front_Det_Rot'])
    INSTRUMENT.REAR_DET_Z = float(logvalues['Rear_Det_Z'])
    INSTRUMENT.REAR_DET_X = float(logvalues['Rear_Det_X'])

    return SCATTER_SAMPLE.getName(), logvalues

########################### 
# Set the scattering can raw workspace
########################### 
_CAN_SETUP = None
_CAN_RUN = ''
def AssignCan(can_run, reload = True, period = -1):
    _printMessage('AssignCan("' + can_run + '")')
    global SCATTER_CAN, _CAN_SETUP, _CAN_RUN, _CAN_N_PERIODS, PERIOD_NOS
    
    __clearPrevious(SCATTER_CAN,others=[SCATTER_SAMPLE,TRANS_SAMPLE,TRANS_CAN,DIRECT_SAMPLE,DIRECT_CAN])
    _CAN_N_PERIODS = -1
    
    if( can_run.startswith('.') or can_run == '' or can_run == None):
        SCATTER_CAN.reset()
        _CAN_RUN = ''
        _CAN_SETUP = None
        return '', '()'

    _CAN_RUN = can_run
    SCATTER_CAN ,reset, logname,filepath, _CAN_N_PERIODS = \
        _assignHelper(can_run, False, reload, period)
    if SCATTER_CAN.getName() == '':
        _issueWarning('Unable to load sans can run, cannot continue.')
        return '','()'
    if reset == True:
        _CAN_SETUP  = None


    try:
        logvalues = INSTRUMENT.load_detector_logs(logname,filepath)
        if logvalues == None:
            _issueWarning("Can logs could not be loaded, using sample values.")
            return SCATTER_CAN.getName(), "()"
    except AttributeError:
        if not INSTRUMENT.name() == 'LOQ' : raise

    PERIOD_NOS["SCATTER_CAN"] = period

    if (INSTRUMENT.name() == 'LOQ'):
        return SCATTER_CAN.getName(), ""

    smp_values = []
    front_det = INSTRUMENT.getDetector('front')
    smp_values.append(INSTRUMENT.FRONT_DET_Z + front_det.z_corr)
    smp_values.append(INSTRUMENT.FRONT_DET_X + front_det.x_corr)
    smp_values.append(INSTRUMENT.FRONT_DET_ROT + front_det.rot_corr)
    rear_det = INSTRUMENT.getDetector('rear')
    smp_values.append(INSTRUMENT.REAR_DET_Z + rear_det.z_corr)
    smp_values.append(INSTRUMENT.REAR_DET_X + rear_det.x_corr)


    # Check against sample values and warn if they are not the same but still continue reduction
    if len(logvalues) == 0:
        return  SCATTER_CAN.getName(), logvalues
    
    can_values = []
    can_values.append(float(logvalues['Front_Det_Z']) + front_det.z_corr)
    can_values.append(float(logvalues['Front_Det_X']) + front_det.x_corr)
    can_values.append(float(logvalues['Front_Det_Rot']) + front_det.rot_corr)
    can_values.append(float(logvalues['Rear_Det_Z']) + rear_det.z_corr)
    can_values.append(float(logvalues['Rear_Det_X']) + rear_det.x_corr)


    det_names = ['Front_Det_Z', 'Front_Det_X','Front_Det_Rot', 'Rear_Det_Z', 'Rear_Det_X']
    for i in range(0, 5):
        if math.fabs(smp_values[i] - can_values[i]) > 5e-04:
            _issueWarning(det_names[i] + " values differ between sample and can runs. Sample = " + str(smp_values[i]) + \
                              ' , Can = ' + str(can_values[i]))
            INSTRUMENT.append_marked(det_names[i])
    
    return SCATTER_CAN.getName(), logvalues

########################### 
# Set the trans sample and measured raw workspaces
########################### 
def TransmissionSample(sample, direct, reload = True, period = -1):
    _printMessage('TransmissionSample("' + sample + '","' + direct + '")')
    global TRANS_SAMPLE, DIRECT_SAMPLE, _TRANS_SAMPLE_N_PERIODS, DIRECT_SAMPLE_N_PERIODS
    
    __clearPrevious(TRANS_SAMPLE,others=[SCATTER_SAMPLE,SCATTER_CAN,TRANS_CAN,DIRECT_SAMPLE,DIRECT_CAN])
    __clearPrevious(DIRECT_SAMPLE,others=[SCATTER_SAMPLE,SCATTER_CAN,TRANS_SAMPLE,TRANS_CAN,DIRECT_CAN])
    
    trans_ws, dummy1, dummy2, dummy3, _TRANS_SAMPLE_N_PERIODS = \
        _assignHelper(sample, True, reload, period)
    TRANS_SAMPLE = trans_ws.getName()
    
    direct_sample_ws, dummy1, dummy2, dummy3, DIRECT_SAMPLE_N_PERIODS = \
        _assignHelper(direct, True, reload, period)
    DIRECT_SAMPLE = direct_sample_ws.getName()
    
    return TRANS_SAMPLE, DIRECT_SAMPLE

########################## 
# Set the trans sample and measured raw workspaces
########################## 
def TransmissionCan(can, direct, reload = True, period = -1):
    _printMessage('TransmissionCan("' + can + '","' + direct + '")')
    global TRANS_CAN, DIRECT_CAN, TRANS_CAN_N_PERIODS, DIRECT_CAN_N_PERIODS
    
    __clearPrevious(TRANS_CAN,others=[SCATTER_SAMPLE,SCATTER_CAN,TRANS_SAMPLE,DIRECT_SAMPLE,DIRECT_CAN])
    __clearPrevious(DIRECT_CAN,others=[SCATTER_SAMPLE,SCATTER_CAN,TRANS_SAMPLE,TRANS_CAN,DIRECT_SAMPLE])

    can_ws, dummy1, dummy2, dummy3, TRANS_CAN_N_PERIODS = \
        _assignHelper(can, True, reload, period)
    TRANS_CAN = can_ws.getName()
    if direct == '' or direct == None:
        DIRECT_CAN, DIRECT_CAN_N_PERIODS = DIRECT_SAMPLE, DIRECT_SAMPLE_N_PERIODS
    else:
        direct_can_ws, dummy1, dummy2, dummy3, DIRECT_CAN_N_PERIODS = \
            _assignHelper(direct, True, reload, period)
        DIRECT_CAN = direct_can_ws.getName()
    return TRANS_CAN, DIRECT_CAN

# Helper function
def _assignHelper(run_string, is_trans, reload = True, period = -1):
    if run_string == '' or run_string.startswith('.'):
        return SANSUtility.WorkspaceDetails('', -1),True,'','', -1
    pieces = run_string.split('.')
    if len(pieces) != 2 :
         _fatalError("Invalid run specified: " + run_string + ". Please use RUNNUMBER.EXT format")
    else:
        run_no = pieces[0]
        ext = pieces[1]
    if run_no == '':
        return SANSUtility.WorkspaceDetails('', -1),True,'','', -1
        
    if INSTRUMENT.name() == 'LOQ':
        field_width = 5
    else:
        field_width = 8
        
    fullrun_no,logname,shortrun_no = padRunNumber(run_no, field_width)
    
    if is_trans:
        wkspname =  shortrun_no + '_trans_' + ext.lower()
    else:
        wkspname =  shortrun_no + '_sans_' + ext.lower()

    if reload == False and mtd.workspaceExists(wkspname):
        return WorkspaceDetails(wkspname, shortrun_no),False,'','', -1

    basename = INSTRUMENT.name() + fullrun_no
    filename = os.path.join(DATA_PATH,basename)
    # Workaround so that the FileProperty does the correct searching of data paths if this file doesn't exist
    if not os.path.exists(filename + '.' + ext):
        filename = basename
    if period <= 0:
        period = 1
    if is_trans:
        try:
            if INSTRUMENT.name() == 'SANS2D' and int(shortrun_no) < 568:
                dimension = SANSUtility.GetInstrumentDetails(INSTRUMENT)[0]
                specmin = dimension*dimension*2
                specmax = specmin + 4
            else:
                specmin = None
                specmax = 8
                
            [filepath, wkspname, nPeriods] = \
               _loadRawData(filename, wkspname, ext, specmin, specmax, period)
        except RuntimeError, err:
            _issueWarning(str(err))
            return SANSUtility.WorkspaceDetails('', -1),True,'','', -1
    else:
        try:
            [filepath, wkspname, nPeriods] = _loadRawData(filename, wkspname, ext, None, None, period)
        except RuntimeError, details:
            _issueWarning(str(details))
            return SANSUtility.WorkspaceDetails('', -1),True,'','', -1


    inWS = SANSUtility.WorkspaceDetails(wkspname, shortrun_no)
    return inWS,True, INSTRUMENT.name() + logname, filepath, nPeriods

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

def __clearPrevious(inWS, others = []):
    if inWS != None:
        if type(inWS) == SANSUtility.WorkspaceDetails:
            inWS = inWS.getName()
        if mtd.workspaceExists(inWS) and (not inWS in others):
            mtd.deleteWorkspace(inWS)

##########################
# Loader function
##########################
def _loadRawData(filename, wsName, ext, spec_min = None, spec_max = None, period=1):
    if ext.lower().startswith('n'):
        alg = LoadNexus(filename + '.' + ext, wsName,SpectrumMin=spec_min,SpectrumMax=spec_max)
    else:
        alg = LoadRaw(filename + '.' + ext, wsName, SpectrumMin = spec_min,SpectrumMax = spec_max)
        LoadSampleDetailsFromRaw(wsName, filename + '.' + ext)

    pWorksp = mtd[wsName]

    if pWorksp.isGroup() :
        #get the number of periods in a group using the fact that each period has a different name
        nNames = len(pWorksp.getNames())
        numPeriods = nNames - 1
        wsName = _leaveSinglePeriod(pWorksp, period)
        pWorksp = mtd[wsName]
    else :
        #if the work space isn't a group there is only one period
        numPeriods = 1
        
    if (period > numPeriods) or (period < 1):
        raise ValueError('_loadRawData: Period number ' + str(period) + ' doesn\'t exist in workspace ' + pWorksp.getName())

    sample_details = pWorksp.getSampleInfo()
    SampleGeometry(sample_details.getGeometryFlag())
    SampleThickness(sample_details.getThickness())
    SampleHeight(sample_details.getHeight())
    SampleWidth(sample_details.getWidth())

    # Return the filepath actually used to load the data
    fullpath = alg.getPropertyValue("Filename")
    return [ os.path.dirname(fullpath), wsName, numPeriods]

##REMOVED STEVE 08 September 2010
def _leaveSinglePeriod(groupW, period):
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
# Return the list of mismatched detector names
def GetMismatchedDetList():
    return INSTRUMENT.get_marked_dets()

#########################
# Limits 
def LimitsR(rmin, rmax):
    _printMessage('LimitsR(' + str(rmin) + ',' +str(rmax) + ')')
    _readLimitValues('L/R ' + str(rmin) + ' ' + str(rmax) + ' 1')

def LimitsWav(lmin, lmax, step, type):
    _printMessage('LimitsWav(' + str(lmin) + ',' + str(lmax) + ',' + str(step) + ','  + type + ')')
    _readLimitValues('L/WAV ' + str(lmin) + ' ' + str(lmax) + ' ' + str(step) + '/'  + type)

def LimitsQ(*args):
    # If given one argument it must be a rebin string
    if len(args) == 1:
        val = args[0]
        if type(val) == str:
            _printMessage("LimitsQ(" + val + ")")
            _readLimitValues("L/Q " + val)
        else:
            _issueWarning("LimitsQ can only be called with a single string or 4 values")
    elif len(args) == 4:
        qmin,qmax,step,step_type = args
        _printMessage('LimitsQ(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(step_type) + ')')
        _readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + step_type)
    else:
        _issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")

def LimitsQXY(qmin, qmax, step, type):
    _printMessage('LimitsQXY(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(type) + ')')
    _readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type)

def LimitsPhi(phimin, phimax, use_mirror=True):
    if use_mirror :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=True)')
        _readLimitValues('L/PHI ' + str(phimin) + ' ' + str(phimax))
    else :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=False)')
        _readLimitValues('L/PHI/NOMIRROR ' + str(phimin) + ' ' + str(phimax))

def Gravity(flag):
    _printMessage('Gravity(' + str(flag) + ')')
    if isinstance(flag, bool) or isinstance(flag, int):
        global GRAVITY
        GRAVITY = flag
    else:
        _issueWarning("Invalid GRAVITY flag passed, try True/False. Setting kept as " + str(GRAVITY))

def TransFit(mode,lambdamin=None,lambdamax=None):
    global TRANS_WAV1, TRANS_WAV2, TRANS_FIT
    mode = mode.upper()

    if mode in TRANS_FIT_OPTIONS.keys():
        TRANS_FIT = TRANS_FIT_OPTIONS[mode]
    else:
        _issueWarning("Invalid fit mode passed to TransFit, using default LOG method")
        TRANS_FIT = mode = TRANS_FIT_DEF

    if lambdamin is None or lambdamax is None:
        _printMessage("TransFit(\"" + str(mode) + "\")")
        TRANS_WAV1 = TRANS_WAV1_FULL
        TRANS_WAV2 = TRANS_WAV2_FULL
    else:
        _printMessage("TransFit(\"" + str(mode) + "\"," + str(lambdamin) + "," + str(lambdamax) + ")")
        TRANS_WAV1 = lambdamin
        TRANS_WAV2 = lambdamax


###################################
# Scaling value
###################################
def _SetScales(scalefactor):
    global RESCALE
    RESCALE = scalefactor * 100.0
##START REMOVED STEVE (ISISCOmmandInterface.py)
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
def SetPhiLimit(phimin,phimax, phimirror=True):
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
          
  	
    global PHIMIN, PHIMAX, PHIMIRROR
    PHIMIN = phimin
    PHIMAX = phimax
    PHIMIRROR = phimirror
##END REMOVED STEVE (ISISCOmmandInterface.py)
##START REMOVED STEVE 13 September 2010 (ISISSANSReductionSteps.py)
def _restore_defaults():

    Mask('MASK/CLEAR')
    Mask('MASK/CLEAR/TIME')
    SetRearEfficiencyFile(None)
    SetFrontEfficiencyFile(None)
    global RMIN,RMAX, DEF_RMIN, DEF_RMAX
    RMIN = RMAX = DEF_RMIN = DEF_RMAX = None
    global WAV1, WAV2, DWAV, Q_REBIN, QXY2, DQY
    WAV1 = WAV2 = DWAV = Q_REBIN = QXY = DQY = None
    global RESCALE, SAMPLE_GEOM, SAMPLE_WIDTH, SAMPLE_HEIGHT, SAMPLE_THICKNESS
    # Scaling values
    RESCALE = 100.  # percent
    SAMPLE_GEOM = 3
    SAMPLE_WIDTH = SAMPLE_HEIGHT = SAMPLE_THICKNESS = 1.0
    front_det = INSTRUMENT.getDetector('rear')
    front_det.z_corr = front_det.y_corr = front_det.x_corr = front_det.rot_corr = 0.0
    rear_det = INSTRUMENT.getDetector('rear')
    rear_det.z_corr = rear_det.x_corr = 0.0
    
    global BACKMON_START, BACKMON_END
    BACKMON_START = BACKMON_END = None
    
    global NORMALISATION_FILE
    NORMALISATION_FILE = None
##START REMOVED STEVE 13 September 2010 (ISISSANSReductionSteps.py)
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
    # A spectrum mask or mask range applied to the rear detector
    if len(parts) == 1:
        spectra = details[4:].lstrip()
        if len(spectra.split()) == 1:
            SPECMASKSTRING_R += ',' + spectra
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
            det_type = detname[0]
            if INSTRUMENT.isDetectorName(det_type) :
                spectra = detname[1]
                if INSTRUMENT.isHighAngleDetector(det_type) :
                    SPECMASKSTRING_F += ',' + spectra
                else:
                    SPECMASKSTRING_R += ',' + spectra
            else:
                _issueWarning('Detector \'' + det_type + '\' not found in currently selected instrument ' + INSTRUMENT.name() + '. Skipping line.')
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
                if INSTRUMENT.detectorExists(detname) :
                    if INSTRUMENT.isHighAngleDetector(detname) :
                        TIMEMASKSTRING_F += ';' + bin_range
                    else:
                        TIMEMASKSTRING_R += ';' + bin_range
                else:
                    _issueWarning('Detector \'' + det_type + '\' not found in currently selected instrument ' + INSTRUMENT.name() + '. Skipping line.')
            else:
                _issueWarning('Unrecognized masking option "' + details + '"')
    else:
        pass
##START REMOVED (ISISReducer.py)
def MaskFile(filename):
    _printMessage('#Opening "'+filename+'"')
    if os.path.isabs(filename) == False:
        filename = os.path.join(USER_PATH, filename)

    if os.path.exists(filename) == False:
        _fatalError("Cannot read mask file '" + filename + "', path does not exist.")
        
    _restore_defaults()

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
            _readMONValues(line)
        
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
        
        elif upper_line.startswith("FIT/TRANS/"):
            params = upper_line[10:].split()
            if len(params) == 3:
                fit_type, lambdamin, lambdamax = params
                TransFit(fit_type, lambdamin, lambdamax)
            elif len(params) == 1:
                TransFit(params[0])
            else:
                _issueWarning('Incorrectly formatted FIT/TRANS line, setting defaults to LOG and full range')
                TransFit(TRANS_FIT_DEF)
        
        else:
            continue

    # Close the handle
    file_handle.close()
    # Check if one of the efficency files hasn't been set and assume the other is to be used
    if DIRECT_BEAM_FILE_R == None and DIRECT_BEAM_FILE_F != None:
        SetRearEfficiencyFile(DIRECT_BEAM_FILE_F)
    if DIRECT_BEAM_FILE_F == None and DIRECT_BEAM_FILE_R != None:
        SetFrontEfficiencyFile(DIRECT_BEAM_FILE_R)
        
    # just print thhe name, remove the path
    filename = os.path.basename(filename)
    global MASKFILE
    MASKFILE = filename
    _printMessage('#Successfully read "'+filename+'"')
    return True

# Read a limit line of a mask file
def _readLimitValues(limit_line):
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
        global WAV1, WAV2, DWAV
        WAV1 = float(minval)
        WAV2 = float(maxval)
        DWAV = float(step_type + step_size)
    elif limit_type.upper() == 'Q':
        global Q_REBIN
        if not rebin_str is None:
            Q_REBIN = rebin_str
        else:
            Q_REBIN = minval + "," + step_type + step_size + "," + maxval
    elif limit_type.upper() == 'QXY':
        global QXY2, DQXY
        QXY2 = float(maxval)
        DQXY = float(step_type + step_size)
    elif limit_type.upper() == 'R':
        global RMIN, RMAX, DEF_RMIN, DEF_RMAX
        RMIN = float(minval)/1000.
        RMAX = float(maxval)/1000.
        DEF_RMIN = RMIN
        DEF_RMAX = RMAX
    elif limit_type.upper() == 'PHI':
        SetPhiLimit(float(minval), float(maxval), True)
    elif limit_type.upper() == 'PHI/NOMIRROR':
        SetPhiLimit(float(minval), float(maxval), False)
    else:
        pass

def _readMONValues(line):
    details = line[4:]

    #MON/LENTH, MON/SPECTRUM and MON/TRANS all accept the INTERPOLATE option
    interpolate = False
    interPlace = details.upper().find('/INTERPOLATE')
    if interPlace != -1 :
        interpolate = True
        details = details[0:interPlace]

    if details.upper().startswith('LENGTH'):
        SuggestMonitorSpectrum(int(details.split()[1]), interpolate)
    
    elif details.upper().startswith('SPECTRUM'):
        SetMonitorSpectrum(int(details.split('=')[1]), interpolate)
    
    elif details.upper().startswith('TRANS'):
        parts = details.split('=')
        if len(parts) < 2 or parts[0].upper() != 'TRANS/SPECTRUM' :
            _issueWarning('Unable to parse MON/TRANS line, needs MON/TRANS/SPECTRUM=')
        SetTransSpectrum(int(parts[1]), interpolate)

    elif 'DIRECT' in details.upper() or details.upper().startswith('FLAT'):
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
                if parts[0].upper() == 'DIRECT':
                    SetRearEfficiencyFile(filepath)
                    SetFrontEfficiencyFile(filepath)
                elif parts[0].upper() == 'HAB':
                    SetFrontEfficiencyFile(filepath)
                elif parts[0].upper() == 'FLAT':
                    SetDetectorFloodFile(filepath)
                else:
                    pass
            elif len(parts) == 2:
                detname = parts[1]
                if detname.upper() == 'REAR':
                    SetRearEfficiencyFile(filepath)
                elif detname.upper() == 'FRONT' or detname.upper() == 'HAB':
                    SetFrontEfficiencyFile(filepath)
                else:
                    _issueWarning('Incorrect detector specified for efficiency file "' + line + '"')
            else:
                _issueWarning('Unable to parse monitor line "' + line + '"')
        else:
            _issueWarning('Unable to parse monitor line "' + line + '"')

def _readDetectorCorrections(details):
    values = details.split()
    det_name = values[0]
    det_axis = values[1]
    shift = float(values[2])

    detector = INSTRUMENT.getDetector(det_name)
    if det_axis == 'X':
        detector.x_corr = shift
    elif det_axis == 'Y':
        detector.y_corr = shift
    elif det_axis == 'Z':
        detector.z_corr = shift
    elif det_axis == 'ROT':
        detector.rot_corr = shift
    else:
        raise NotImplemented('Detector correction on "'+det_axis+'" is not supported')
##END REMOVED (ISISReducer.py)
##START REMOVED STEVE 13 September 2010 (ISISSANSReductionSteps.py)
def SetSampleOffset(value):
    INSTRUMENT.set_sample_offset(value)

def SetMonitorSpectrum(specNum, interp=False):
    INSTRUMENT.set_incident_mon(specNum)
    #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
    if interp :
        INSTRUMENT.set_interpolating_norm()

def SuggestMonitorSpectrum(specNum, interp=False):
    INSTRUMENT.suggest_incident_mntr(specNum)
    #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
    if interp :
        INSTRUMENT.suggest_interpolating_norm()

def SetTransSpectrum(specNum, interp=False):
    INSTRUMENT.incid_mon_4_trans_calc = int(specNum)
    #if interpolate is stated once in the file, that is enough it wont be unset (until a file is loaded again)
    if interp :
        INSTRUMENT.use_interpol_trans_calc = True

def SetRearEfficiencyFile(filename):
    global DIRECT_BEAM_FILE_R
    DIRECT_BEAM_FILE_R = filename
    
def SetFrontEfficiencyFile(filename):
    global DIRECT_BEAM_FILE_F
    DIRECT_BEAM_FILE_F = filename

def SetDetectorFloodFile(filename):
    global NORMALISATION_FILE
    NORMALISATION_FILE = filename

def displayMaskFile():
    print '-- Mask file defaults --'
    print '    Wavelength range: ',WAV1, WAV2, DWAV
    print '    Q range: ', Q_REBIN
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
    if SCATTER_CAN.getName() != '' and _CAN_SETUP == None:
        _CAN_SETUP = _init_run(SCATTER_CAN, [xcentre, ycentre], True)

    # Instrument specific information using function in utility file
    global DIMENSION, SPECMIN, SPECMAX, INSTRUMENT
    DIMENSION, SPECMIN, SPECMAX  = SANSUtility.GetInstrumentDetails(INSTRUMENT)

    return _SAMPLE_SETUP, _CAN_SETUP
##END REMOVED STEVE 13 September 2010 (ISISSANSReductionSteps.py)
##START REMOVED STEVE 14 September 2010 (ISISSANSReductionSteps.py)
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
##END REMOVED STEVE 14 September 2010 (ISISSANSReductionSteps.py)
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
            mantid.deleteWorkspace(final_workspace)
                
    # Crop Workspace to remove leading and trailing zeroes
    if finding_centre == False:
        # Replaces NANs with zeroes
        ReplaceSpecialValues(InputWorkspace = final_workspace, OutputWorkspace = final_workspace, NaNValue="0", InfinityValue="0")
        if CORRECTION_TYPE == '1D':
            SANSUtility.StripEndZeroes(final_workspace)
        # Store the mask file within the final workspace so that it is saved to the CanSAS file
        AddSampleLog(final_workspace, "UserFile", MASKFILE)
    else:
        quadrants = {1:'Left', 2:'Right', 3:'Up',4:'Down'}
        for key, value in quadrants.iteritems():
            old_name = final_workspace + '_' + str(key)
            RenameWorkspace(old_name, value)
            AddSampleLog(value, "UserFile", MASKFILE)

    # Revert the name change so that future calls with different wavelengths get the correct name
    sample_setup.setReducedWorkspace(wsname_cache)
    return final_workspace
##REMOVED STEVE 08 September 2010
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
        final_ws = raw_ws.getName().split('_')[0]
        final_ws += INSTRUMENT.cur_detector().name('short')
        final_ws += '_' + CORRECTION_TYPE

    # Put the components in the correct positions
    currentDet = INSTRUMENT.cur_detector().name() 
    maskpt_rmin, maskpt_rmax = INSTRUMENT.set_component_positions(raw_ws.getName(), beamcoords[0], beamcoords[1])
    
    # Create a run details object
    if emptycell == True:
        return SANSUtility.RunDetails(raw_ws, final_ws, TRANS_CAN, DIRECT_CAN, maskpt_rmin, maskpt_rmax, 'can')
    else:
        return SANSUtility.RunDetails(raw_ws, final_ws, TRANS_SAMPLE, DIRECT_SAMPLE, maskpt_rmin, maskpt_rmax, 'sample')
##REMOVED STEVE 08 September 2010
##START REMOVED ISISReductionSteps
# Setup the transmission workspace
##
def CalculateTransmissionCorrection(run_setup, lambdamin, lambdamax, use_def_trans):
    trans_raw = run_setup.getTransRaw()
    direct_raw = run_setup.getDirectRaw()
    if trans_raw == '' or direct_raw == '':
        return None

    if use_def_trans == DefaultTrans:
        wavbin = str(TRANS_WAV1_FULL) + ',' + str(DWAV) + ',' + str(TRANS_WAV2_FULL)
        translambda_min = TRANS_WAV1_FULL
        translambda_max = TRANS_WAV2_FULL
    else:
        translambda_min = TRANS_WAV1
        translambda_max = TRANS_WAV2
        wavbin = str(lambdamin) + ',' + str(DWAV) + ',' + str(lambdamax)

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
        if INSTRUMENT.name() == 'LOQ':
            # Change the instrument definition to the correct one in the LOQ case
            LoadInstrument(trans_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
            LoadInstrument(direct_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
            
            trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw,
                '1,2', BACKMON_START, BACKMON_END, wavbin, 
                INSTRUMENT.use_interpol_trans_calc, True)
            
            direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw,
                '1,2', BACKMON_START, BACKMON_END, wavbin,
                INSTRUMENT.use_interpol_trans_calc, True)
            
            CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws, MinWavelength = translambda_min, MaxWavelength =  translambda_max, \
                                  FitMethod = fit_type, OutputUnfittedData=True)
        else:
            trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw,
                '1,2', BACKMON_START, BACKMON_END, wavbin,
                INSTRUMENT.use_interpol_trans_calc, False)
            
            direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw,
                '1,2', BACKMON_START, BACKMON_END, wavbin,
                INSTRUMENT.use_interpol_trans_calc, False)
            
            CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws,
                INSTRUMENT.incid_mon_4_trans_calc, INSTRUMENT.trans_monitor,
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

    if use_def_trans == DefaultTrans:
        tmp_ws = 'trans_' + run_setup.getSuffix() + '_' + str(lambdamin) + '_' + str(lambdamax)
        CropWorkspace(result, tmp_ws, XMin = str(lambdamin), XMax = str(lambdamax))
        return tmp_ws
    else: 
        return result
##END REMOVE (ISISReductionSteps
##REMOVED STEVE 08 September 2010
##
# Apply the spectrum and time masks to one detector in a given workspace
##
def _applyMasking(workspace, firstspec, dimension, inst, orientation=SANSUtility.Orientation.Horizontal,limitphi = True, useActiveDetector=True):

    #Time mask
    SANSUtility.MaskByBinRange(workspace,TIMEMASKSTRING)
    
    #this function only masks one detector, find out which one that should be
    setLowAngle = inst.lowAngDetSet
    if not useActiveDetector :
        setLowAngle = not inst.lowAngDetSet
        
    if setLowAngle :
        specstring = SPECMASKSTRING_R
        timestring = TIMEMASKSTRING_R
    else:
        specstring = SPECMASKSTRING_F
        timestring = TIMEMASKSTRING_F

    speclist = SANSUtility.ConvertToSpecList(specstring, firstspec, dimension,orientation) 
    _printMessage(str(speclist))
    SANSUtility.MaskBySpecNumber(workspace, speclist)

    SANSUtility.MaskByBinRange(workspace, timestring)
##END REMOVED STEVE
##START REMOVED STEVE
    # Finally apply masking to get the correct phi range
    if limitphi == True:
        # Detector has been moved such that beam centre is at [0,0,0]
        SANSUtility.LimitPhi(workspace, [0,0,0], PHIMIN,PHIMAX,PHIMIRROR)
##END REMOVED STEVE
##START REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)
def Correct(run_setup, wav_start, wav_end, use_def_trans, finding_centre = False):
    '''Performs the data reduction steps'''
    global SPECMIN, SPECMAX
    sample_raw = run_setup.getRawWorkspace()
#but does the full run still exist at this point, doesn't matter  I'm changing the meaning of RawWorkspace get all references to it
#but then what do we call the workspaces?

#    period = run_setup.getPeriod()
    orientation = orientation=SANSUtility.Orientation.Horizontal
    if INSTRUMENT.name() == "SANS2D":
        base_runno = sample_raw.getRunNumber()
        if base_runno < 568:
            INSTRUMENT.set_incident_mon(73730)
            orientation=SANSUtility.Orientation.Vertical
            if INSTRUMENT.cur_detector().name() == 'front-detector':
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
    montorSpecNum = INSTRUMENT.get_incident_mon()
    _printMessage('monitor ' + str(montorSpecNum), True)
    sample_name = sample_raw.getName()
    # Get the monitor ( StartWorkspaceIndex is off by one with cropworkspace)
    CropWorkspace(sample_name, monitorWS,
        StartWorkspaceIndex= montorSpecNum-1, EndWorkspaceIndex=montorSpecNum-1)
    if INSTRUMENT.name() == 'LOQ':
        RemoveBins(monitorWS, monitorWS, '19900', '20500',
            Interpolation="Linear")
    
    # Remove flat background
    if BACKMON_START != None and BACKMON_END != None:
        FlatBackground(monitorWS, monitorWS, StartX = BACKMON_START, EndX = BACKMON_END, WorkspaceIndexList = '0')
    
    final_result = run_setup.getReducedWorkspace()
    to_crop = sample_name
    if not NORMALISATION_FILE is None:
        if not NORMALISATION_FILE == "":
            to_crop = final_result
            CorrectToFile(sample_name, NORMALISATION_FILE, to_crop, 'SpectrumNumber', 'Divide')

    # Get the bank we are looking at
    CropWorkspace(to_crop, final_result,
        StartWorkspaceIndex = (SPECMIN - 1), EndWorkspaceIndex = str(SPECMAX - 1))
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

    _applyMasking(final_result, SPECMIN, DIMENSION, INSTRUMENT, orientation)
    ####################################################################################

    ######################## Unit change and rebin #####################################
    # Convert all of the files to wavelength and rebin
    # ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
    ConvertUnits(monitorWS, monitorWS, "Wavelength")
    wavbin =  str(wav_start) + "," + str(DWAV) + "," + str(wav_end)
    if INSTRUMENT.is_interpolating_norm() :
        InterpolatingRebin(monitorWS, monitorWS,wavbin)
    else :
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
    if INSTRUMENT.lowAngDetSet :
        CorrectToFile(tmpWS, DIRECT_BEAM_FILE_R, tmpWS, "Wavelength", "Divide")
    else:
        CorrectToFile(tmpWS, DIRECT_BEAM_FILE_F, tmpWS, "Wavelength", "Divide")
    ###################################################################################
        
    ############################# Scale by volume #####################################
    scalefactor = RESCALE
    # Data reduced with Mantid is a factor of ~pi higher than colette.
    # For LOQ only, divide by this until we understand why.
    if INSTRUMENT.name() == 'LOQ':
        rescaleToColette = math.pi
        scalefactor /= rescaleToColette
    
    SANSUtility.ScaleByVolume(tmpWS, scalefactor, SAMPLE_GEOM, SAMPLE_WIDTH, SAMPLE_HEIGHT, SAMPLE_THICKNESS)
    ################################################## ################################
        
    ################################ Correction in Q space ############################
    # 1D
    if CORRECTION_TYPE == '1D':
        if finding_centre == True:
            GroupIntoQuadrants(tmpWS, final_result, maskpt_rmin[0], maskpt_rmin[1], Q_REBIN)
            return
        else:
            Q1D(tmpWS,final_result,final_result,Q_REBIN, AccountForGravity=GRAVITY)
    # 2D    
    else:
        if finding_centre == True:
            raise Exception('Running center finding in 2D analysis is not possible, set1D() first.') 
        # Run 2D algorithm
        Qxy(tmpWS, final_result, QXY2, DQXY)

    mantid.deleteWorkspace(tmpWS)
    return
##END REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)
##REMOVED STEVE 08 September 2010 (ISISCommandInterface.py)
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
    mantid.deleteWorkspace(reduced_ws)

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
                        
    try :
        if RESIDUE_GRAPH is None or (not RESIDUE_GRAPH in appwidgets()):
            RESIDUE_GRAPH = plotSpectrum('Left', 0)
            mergePlots(RESIDUE_GRAPH, plotSpectrum(['Right','Up'],0))
            mergePlots(RESIDUE_GRAPH, plotSpectrum(['Down'],0))
        RESIDUE_GRAPH.activeLayer().setTitle("Itr " + str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))
    except :
        #if plotting is not available it probably means we are running outside a GUI, in which case do everything but don't plot
        pass
        
    mantid.sendLogMessage("::SANS::Itr: "+str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))              
    return residueX, residueY
##END REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)
def RunReduction(coords):
    '''Compute the value of (L-R)^2+(U-D)^2 a circle split into four quadrants'''
    global XVAR_PREV, YVAR_PREV
    xcentre = coords[0]
    ycentre= coords[1]
    
    xshift = -xcentre + XVAR_PREV
    yshift = -ycentre + YVAR_PREV
    XVAR_PREV = xcentre
    YVAR_PREV = ycentre

    # Do the correction
    if xshift != 0.0 or yshift != 0.0:
        currentDet = INSTRUMENT.cur_detector().name()
        MoveInstrumentComponent(SCATTER_SAMPLE.getName(),
            ComponentName=currentDet, X=xshift, Y=yshift, RelativePosition="1")
        if SCATTER_CAN.getName() != '':
            MoveInstrumentComponent(SCATTER_CAN.getName(),
             ComponentName=currentDet, X=xshift, Y=yshift, RelativePosition="1")
##START REMOVED
    _SAMPLE_SETUP.setMaskPtMin([0.0,0.0])
    _SAMPLE_SETUP.setMaskPtMax([xcentre, ycentre])
    if _CAN_SETUP != None:
        _CAN_SETUP.setMaskPtMin([0.0, 0.0])
        _CAN_SETUP.setMaskPtMax([xcentre, ycentre])
##END REMOVED

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
    XSTEP = YSTEP = INSTRUMENT.cen_find_step
    
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
    _assignHelper(_SAMPLE_RUN, False, PERIOD_NOS["SCATTER_SAMPLE"])
    _SAMPLE_SETUP = None
    if _CAN_RUN != '':
        _assignHelper(_CAN_RUN, False, PERIOD_NOS["SCATTER_CAN"])
        global _CAN_SETUP
        _CAN_SETUP = None
    
    RMIN = DEF_RMIN
    RMAX = DEF_RMAX
##START REMOVED STEVE 08 September 2010 (ISISCommandInterface.py)
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
    inst_name = INSTRUMENT.name()
    top_layer = 'CurrentMask'
    inst = eval('SANSInsts.'+inst_name+'("'+top_layer+'")')

    if RMIN > 0.0: 
        SANSUtility.MaskInsideCylinder(top_layer, RMIN, XBEAM_CENTRE, YBEAM_CENTRE)
    if RMAX > 0.0:
        SANSUtility.MaskOutsideCylinder(top_layer, RMAX, 0.0, 0.0)

    dimension = SANSUtility.GetInstrumentDetails(inst)[0]

    #_applyMasking() must be called on both detectors, the detector is specified by passing the index of it's first spectrum
    #start with the currently selected detector
    firstSpec1 = inst.cur_detector().get_first_spec_num()
    _applyMasking(top_layer, firstSpec1, dimension,  inst, SANSUtility.Orientation.Horizontal, False, True)
    #now the other detector
    firstSpec2 = inst.other_detector().get_first_spec_num()
    _applyMasking(top_layer, firstSpec2, dimension, inst, SANSUtility.Orientation.Horizontal, False, False)
    
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
    if file_1 != format and file_2 != format:
        script += '[COLETTE]  TRANSMISSION/SAMPLE/MEASURED ' + file_1 + ' ' + file_2 + '\n'
    file_1 = inputdata['can_sans'] + format
    if file_1 != format:
        script +='[COLETTE]  ASSIGN/CAN ' + file_1 + '\n'
    file_1 = inputdata['can_trans'] + format
    file_2 = inputdata['can_direct_beam'] + format
    if file_1 != format and file_2 != format:
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
    # For the moment treat the rebin string as min/max/step
    qbins = q_REBEIN.split(",")
    nbins = len(qbins)
    if CORRECTION_TYPE == '1D':
        script += '[COLETTE]  LIMIT/Q ' + str(qbins[0]) + ' ' + str(qbins[nbins-1]) + '\n'
        dq = float(qbins[1])
        if dq <  0:
            script += '[COLETTE]  STEP/Q/LOGARITHMIC ' + str(dq)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/Q/LINEAR ' + str(dq) + '\n'
    else:
        script += '[COLETTE]  LIMIT/QXY ' + str(0.0) + ' ' + str(QXY2) + '\n'
        if DQXY <  0:
            script += '[COLETTE]  STEP/QXY/LOGARITHMIC ' + str(DQXY)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/QXY/LINEAR ' + str(DQXY) + '\n'
    
    # Correct
    script += '[COLETTE] CORRECT\n'
    if plotresults:
        script += '[COLETTE]  DISPLAY/HISTOGRAM ' + reduced + '\n'
    if savepath != '':
        script += '[COLETTE]  WRITE/LOQ ' + reduced + ' ' + savepath + '\n'
        
    return script
##END REMOVED STEVE 13 September 2010 (ISISCommandInterface.py)