"""
    Commands that are specific to ISIS SANS, in addition to those in CommandInterface.py
    Documentation for running a conversion from neutron count files in time-of-flight
    into Q-space can be found at http://www.mantidproject.org/SANS
"""
import os

import SANSInsts
from CommandInterface import *
import ISISReductionSteps
import SANSReductionSteps
import ISISReducer

#remove the following
DEL__FINDING_CENTRE_ = False

singleton = None
def use_singleton():
    """
        Sets up the ISISCommandInterface by setting the variable
        singleton equal to the reference held in ReductionSingleton.
        The SANS GUI doesn't use the singleton but a reducer that it
        sets up it's self
    """
    global singleton
    ReductionSingleton()
    singleton = ReductionSingleton().reference()

def _active_red(reducer):
    """
        Checks if the object that was passed is None, if it
        is it returns a reference to the reduction singleton
        otherwise it returns the passed object
    """
    if reducer is None:
        if singleton:
            return singleton
        else:
            raise AttributeError('No active reduction object found, either you the function in ISISCommandInterface or specify the reducer')
    return reducer

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

_VERBOSE_ = False


def SetVerboseMode(state):
#TODO: this needs to be on the reducer
    _VERBOSE_ = state

# Print a message and log it if the 
def _printMessage(msg, log = True, no_console=False):
    if log == True and _VERBOSE_ == True:
        mantid.sendLogMessage('::SANS::' + msg)
    if not no_console:
        if singleton:
            print msg
    
def issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    ISISReductionSteps._issueWarning(msg)
                
def UserPath(path):
    _printMessage('UserPath("' + path + '") #Will look for mask file here')
    ReductionSingleton().user_file_path = path
        
def SANS2D(reducer=None):
    _printMessage('SANS2D()', no_console=reducer)
    instrument = SANSInsts.SANS2D()
        
    _active_red(reducer).set_instrument(instrument)

def LOQ(reducer=None):
    _printMessage('LOQ()', no_console=reducer)
    instrument = SANSInsts.LOQ()

    _active_red(reducer).set_instrument(instrument)
    
def Detector(det_name):
    _printMessage('Detector("' + det_name + '")')
    ReductionSingleton().instrument.setDetector(det_name)
    
def Mask(details, reducer=None):
    _printMessage('Mask("' + details + '")', no_console=reducer)
    _active_red(reducer).mask.parse_instruction(details)
    
def MaskFile(file_name):
    _printMessage('#Opening "'+file_name+'"')
    ReductionSingleton().user_settings = ISISReductionSteps.UserFile(
        file_name)
    status = ReductionSingleton().user_settings.execute(
        ReductionSingleton(), None)
    _printMessage('#Success reading "'+file_name+'"'+' is '+str(status))
    return status
    
def SetMonitorSpectrum(specNum, interp=False, reducer=None):
    _active_red(reducer).set_monitor_spectrum(specNum, interp)

def SuggestMonitorSpectrum(specNum, interp=False):  
    ReductionSingleton().suggest_monitor_spectrum(specNum, interp)
    
def SetTransSpectrum(specNum, interp=False, reducer=None):
    _active_red(reducer).set_trans_spectrum(specNum, interp)
      
def SetPhiLimit(phimin,phimax, phimirror=True):
    ReductionSingleton().set_phi_limit(phimin, phimax, phimirror)
    
def SetSampleOffset(value, reducer=None):
    _active_red(reducer).instrument.set_sample_offset(value)
    
def Gravity(flag, reducer=None):
    _printMessage('Gravity(' + str(flag) + ')', no_console=reducer)
    _active_red(reducer).to_Q.set_gravity(flag)
    
def TransFit(mode,lambdamin=None,lambdamax=None, reducer=None):
    if not reducer:
        message = str(mode)
        if lambdamin: message += str(lambdamin)
        if lambdamax: message += str(lambdamax)
        _printMessage("TransFit(\"" + message + "\")", no_console=reducer)
    _active_red(reducer).set_trans_fit(lambdamin, lambdamax, mode)
    
def AssignCan(can_run, reload = True, period = -1, reducer=None):
    _printMessage('AssignCan("' + can_run + '")', no_console=reducer)
    _active_red(reducer).set_background(can_run, reload = reload, period = period)
    return _active_red(reducer).background_subtracter.assign_can(
                                                    _active_red(reducer))
    
def TransmissionSample(sample, direct, reload = True, period = -1, reducer=None):
    _printMessage('TransmissionSample("' + sample + '","' + direct + '")', no_console=reducer)
    _active_red(reducer).set_trans_sample(sample, direct, reload = True, period = -1)
    return _active_red(reducer).samp_trans_load.execute(
                                        _active_red(reducer), None)

def TransmissionCan(can, direct, reload = True, period = -1, reducer=None):
    _printMessage('TransmissionCan("' + can + '","' + direct + '")', no_console=reducer)
    _active_red(reducer).set_trans_can(can, direct, reload = True, period = -1)
    return _active_red(reducer).can_trans_load.execute(
                                            _active_red(reducer), None)
    
def AssignSample(sample_run, reload = True, period = -1, reducer=None):
    _printMessage('AssignSample("' + sample_run + '")', no_console=reducer)
    reducer = _active_red(reducer)

    reducer.data_loader = ISISReductionSteps.LoadSample(
                                            sample_run)
    reducer.load_set_options(reload, period)

    sample_wksp, logs = reducer.data_loader.execute(
                                            _active_red(reducer), None)
    reducer.append_data_file(sample_run, sample_wksp)
    return sample_wksp, logs

def SetCentre(XVAL, YVAL, reducer=None):
    _printMessage('SetCentre(' + str(XVAL) + ',' + str(YVAL) + ')', no_console=reducer)

    _active_red(reducer).set_beam_finder(SANSReductionSteps.BaseBeamFinder(float(XVAL)/1000.0, float(YVAL)/1000.0))


def SetSampleOffset(value, reducer=None):
    _active_red(reducer).instrument.set_sample_offset(value)
    
def GetMismatchedDetList(reducer=None):
    """
        Return the list of mismatched detector names
    """
    return _active_red(reducer).instrument.get_marked_dets()

def WavRangeReduction(wav_start = None, wav_end = None, full_trans_wav = None, reducer=None):
    reduction_chain = _active_red(reducer)
    _printMessage('WavRangeReduction(' + str(wav_start) + ',' + str(wav_end) + ','+str(full_trans_wav)+')', no_console=reducer)
    _printMessage('Running reduction for ' + str(reduction_chain.to_wavelen))

    if not full_trans_wav is None:
        reduction_chain.full_trans_wav = full_trans_wav

    reduction_chain.to_wavelen.set_range(wav_start, wav_end)

    

    if DEL__FINDING_CENTRE_ == True:
        final_workspace = wsname_cache.split('_')[0] + '_quadrants'
        if reduction_chain.to_Q.output_type == '1D':
                GroupIntoQuadrants(tmpWS, final_result, maskpt_rmin[0], maskpt_rmin[1], Q_REBIN)
                return
    #run the chain that has been created so far
    if not reducer:
        #run the chain and kill it
        result = ReductionSingleton.run()
    #refresh our reference, if we have one
        use_singleton()
    else:
        result = reduction_chain._reduce()    
        
    return result

    
def _SetWavelengthRange(start, end):
    ReductionSingleton().to_wavelen.set_range(start, end)


def Set1D():
    _printMessage('Set1D()')
    ReductionSingleton().to_Q.output_type = '1D'

def Set2D():
    _printMessage('Set2D()')
    ReductionSingleton().to_Q.output_type = '2D'

def SetRearEfficiencyFile(filename):
    rear_det = ReductionSingleton().instrument.getDetector('rear')
    rear_det.correction_file = filename

def SetFrontEfficiencyFile(filename):
    front_det = ReductionSingleton().instrument.getDetector('front')
    front_det.correction_file = filename

def SetDetectorFloodFile(filename, reducer=None):
    _active_red(reducer).flood_file.set_filename(filename)

def displayUserFile():
    print '-- Mask file defaults --'
    print ReductionSingleton().to_wavlen
    print ReductionSingleton().Q_string()
    print correction_files()
    print '    direct beam file rear:',
    print ReductionSingleton().instrument.detector_file('rear')
    print '    direct beam file front:',
    print ReductionSingleton().instrument.detector_file('front')
    print ReductionSingleton().mask

def displayMaskFile():
    displayUserFile()

def displayGeometry():
    [x, y] = ReductionSingleton()._beam_finder.get_beam_center()
    print 'Beam centre: [' + str(x) + ',' + str(y) + ']'
    print ReductionSingleton().geometry

def SetPhiLimit(phimin,phimax, phimirror=True):
    maskStep = ReductionSingleton().get_mask()
    #a beam centre of [0,0,0] makes sense if the detector has been moved such that beam centre is at [0,0,0]
    maskStep.set_phi_limit(phimin, phimax, phimirror)
    
def LimitsPhi(phimin, phimax, use_mirror=True, reducer=None):
    '''
        !!DEPRECIATED by the function above, remove!!
        need to remove from SANSRunWindow.cpp
    '''
    reducer = _active_red(reducer)
    settings = reducer.user_settings
    if use_mirror :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=True)', no_console=reducer)
        settings.readLimitValues('L/PHI ' + str(phimin) + ' ' + str(phimax), reducer)
    else :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=False)', no_console=reducer)
        settings.readLimitValues('L/PHI/NOMIRROR ' + str(phimin) + ' ' + str(phimax), reducer)

def LimitsR(rmin, rmax, reducer=None):
    _printMessage('LimitsR(' + str(rmin) + ',' +str(rmax) + ')', reducer)
    reducer = _active_red(reducer)
    settings = reducer.user_settings
    settings.readLimitValues('L/R ' + str(rmin) + ' ' + str(rmax) + ' 1', reducer)

def LimitsWav(lmin, lmax, step, type, reducer=None):
    _printMessage('LimitsWav(' + str(lmin) + ',' + str(lmax) + ',' + str(step) + ','  + type + ')', no_console=reducer)
    reducer = _active_red(reducer)
    settings = reducer.user_settings
    settings.readLimitValues('L/WAV ' + str(lmin) + ' ' + str(lmax) + ' ' + str(step) + '/'  + type, reducer)

def LimitsQ(*args):
    settings = ReductionSingleton().user_settings
    # If given one argument it must be a rebin string
    if len(args) == 1:
        val = args[0]
        if type(val) == str:
            _printMessage("LimitsQ(" + val + ")")
            settings.readLimitValues("L/Q " + val, ReductionSingleton())
        else:
            _issueWarning("LimitsQ can only be called with a single string or 4 values")
    elif len(args) == 4:
        qmin,qmax,step,step_type = args
        _printMessage('LimitsQ(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(step_type) + ')')
        settings.readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + step_type, ReductionSingleton())
    else:
        _issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")

def LimitsQXY(qmin, qmax, step, type, reducer=None):
    _printMessage('LimitsQXY(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(type) + ')', no_console=reducer)
    reducer = _active_red(reducer)
    settings = reducer.user_settings

    settings.readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type, reducer)

# These variables keep track of the centre coordinates that have been used so that we can calculate a relative shift of the
# detector
XVAR_PREV = 0.0
YVAR_PREV = 0.0
ITER_NUM = 0
RESIDUE_GRAPH = None

# Create a workspace with a quadrant value in it 
def _create_quadrant(reduced_ws, rawcount_ws, quadrant, xcentre, ycentre, q_bins, output):
    # Need to create a copy because we're going to mask 3/4 out and that's a one-way trip
    CloneWorkspace(reduced_ws,output)
    objxml = SANSUtility.QuadrantXML([xcentre, ycentre, 0.0], RMIN, RMAX, quadrant)
    # Mask out everything outside the quadrant of interest
    MaskDetectorsInShape(output,objxml)
    # Q1D ignores masked spectra/detectors. This is on the InputWorkspace, so we don't need masking of the InputForErrors workspace
    Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=ReductionSingleton().to_Q.get_gravity())

    flag_value = -10.0
    ReplaceSpecialValues(InputWorkspace=output,OutputWorkspace=output,NaNValue=flag_value,InfinityValue=flag_value)
    if ReductionSingleton().to_Q.output_type == '1D':
        SANSUtility.StripEndZeroes(output, flag_value)

# Create 4 quadrants for the centre finding algorithm and return their names
def _group_into_quadrants(reduced_ws, final_result, xcentre, ycentre, q_bins):
    tmp = 'quad_temp_holder'
    pieces = ['Left', 'Right', 'Up', 'Down']
    to_group = ''
    counter = 0
    for q in pieces:
        counter += 1
        to_group += final_result + '_' + str(counter) + ','
        _create_quadrant(reduced_ws, final_result, q, xcentre, ycentre, q_bins, final_result + '_' + str(counter))

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

def PlotResult(workspace, reducer=None):
    if not reducer:
        reducer = _active_red(reducer)

    if reducer.to_Q.output_type == '1D':
        plotSpectrum(workspace,0)
    else:
        qApp.mantidUI.importMatrixWorkspace(workspace).plotGraph2D()

##################### View mask details #####################################################

def ViewCurrentMask():
    ReductionSingleton().ViewCurrentMask()

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
    script += '[COLETTE]  LIMIT/RADIUS ' + str(ReductionSingleton().mask.min_radius)
    script += ' ' + str(ReductionSingleton().mask.max_radius) + '\n'
    script += '[COLETTE]  LIMIT/WAVELENGTH ' + ReductionSingleton().to_wavelen.get_range() + '\n'
    if DWAV <  0:
        script += '[COLETTE]  STEP/WAVELENGTH/LOGARITHMIC ' + str(ReductionSingleton().to_wavelen.w_step)[1:] + '\n'
    else:
        script += '[COLETTE]  STEP/WAVELENGTH/LINEAR ' + str(ReductionSingleton().to_wavelen.w_step) + '\n'
    # For the moment treat the rebin string as min/max/step
    qbins = q_REBEIN.split(",")
    nbins = len(qbins)
    if ReductionSingleton().to_Q.output_type == '1D':
        script += '[COLETTE]  LIMIT/Q ' + str(qbins[0]) + ' ' + str(qbins[nbins-1]) + '\n'
        dq = float(qbins[1])
        if dq <  0:
            script += '[COLETTE]  STEP/Q/LOGARITHMIC ' + str(dq)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/Q/LINEAR ' + str(dq) + '\n'
    else:
        script += '[COLETTE]  LIMIT/QXY ' + str(0.0) + ' ' + str(ReductionSingleton().QXY2) + '\n'
        if ReductionSingleton().DQXY <  0:
            script += '[COLETTE]  STEP/QXY/LOGARITHMIC ' + str(ReductionSingleton().DQXY)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/QXY/LINEAR ' + str(ReductionSingleton().DQXY) + '\n'
    
    # Correct
    script += '[COLETTE] CORRECT\n'
    if plotresults:
        script += '[COLETTE]  DISPLAY/HISTOGRAM ' + reduced + '\n'
    if savepath != '':
        script += '[COLETTE]  WRITE/LOQ ' + reduced + ' ' + savepath + '\n'
        
    return script

ReductionSingleton.clean(ISISReducer.ISISReducer)

#this is like a #define I'd like to get rid of it because it means nothing here
DefaultTrans = 'True'
NewTrans = 'False'