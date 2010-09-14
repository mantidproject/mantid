"""
    Command interface for ISIS SANS instruments.
    
    As commands in the common CommandInterface module are found to be ISIS-specific, 
    they should be moved here.
"""
import os

import SANSInsts
from CommandInterface import *
import ISISReducer

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

_NOPRINT_ = False
_VERBOSE_ = False



def SetNoPrintMode(quiet = True):
    _NOPRINT_ = quiet

def SetVerboseMode(state):
    _VERBOSE_ = state

# Print a message and log it if the 
def _printMessage(msg, log = True):
    if log == True and _VERBOSE_ == True:
        mantid.sendLogMessage('::SANS::' + msg)
    if _NOPRINT_ == True: 
        return
    print msg
    
def _issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    mantid.sendLogMessage('::SANS::Warning: ' + msg)
                
def UserPath(path):
    ReductionSingleton().set_user_path(path)
        
def SANS2D():
    Clear(ISISReducer.ISISReducer)
    instrument = SANSInsts.SANS2D()
    
    #TODO: this should probably be part of __init__. Leave it for now
    # so we don't create problems with the current code.
    instrument.TRANS_WAV1_FULL = instrument.TRANS_WAV1 = 2.0
    instrument.TRANS_WAV2_FULL = instrument.TRANS_WAV2 = 14.0
    instrument.lowAngDetSet = True
    
    ReductionSingleton().set_instrument(instrument)

def LOQ():
    Clear(ISISReducer.ISISReducer)
    instrument = SANSInsts.LOQ()
    
    #TODO: refactor this away
    instrument.TRANS_WAV1_FULL = instrument.TRANS_WAV1 = 2.2
    instrument.TRANS_WAV2_FULL = instrument.TRANS_WAV2 = 10.0
    instrument.lowAngDetSet = True
    
    ReductionSingleton().set_instrument(instrument)
    
def Detector(det_name):
    ReductionSingleton().instrument.setDetector(det_name)
    
def Mask(details):
    _printMessage('Mask("' + details + '")')
    ReductionSingleton().mask(details)
    
def MaskFile(file_name):
    ReductionSingleton().read_mask_file(file_name)
    
def SetMonitorSpectrum(specNum, interp=False):
    ReductionSingleton().set_monitor_spectrum(specNum, interp)

def SuggestMonitorSpectrum(specNum, interp=False):  
    ReductionSingleton().suggest_monitor_spectrum(specNum, interp)
    
def SetTransSpectrum(specNum, interp=False):
    ReductionSingleton().set_trans_spectrum(specNum, interp)
      
def SetPhiLimit(phimin,phimax, phimirror=True):
    ReductionSingleton().set_phi_limit(phimin, phimax, phimirror)
    
def SetSampleOffset(value):
    ReductionSingleton().instrument.set_sample_offset(value)
    
def Gravity(flag):
    ReductionSingleton().set_gravity(flag)
    
def TransFit(mode,lambdamin=None,lambdamax=None):
    ReductionSingleton().set_trans_fit(lambda_min=lambdamin, 
                                       lambda_max=lambdamax, 
                                       fit_method=mode)
    
def AssignCan(can_run, reload = True, period = -1):
    ReductionSingleton().set_background(can_run, reload = reload, period = period)
    
def TransmissionSample(sample, direct, reload = True, period = -1):
    ReductionSingleton().set_trans_sample(sample, direct, reload = True, period = -1)
    
def TransmissionCan(can, direct, reload = True, period = -1):
    _printMessage('TransmissionCan("' + can + '","' + direct + '")')
    ReductionSingleton().set_trans_can(can, direct, reload = True, period = -1)
    
def AssignSample(sample_run, reload = True, period = -1):
    _printMessage('AssignSample("' + sample_run + '")')
    ReductionSingleton().append_data_file(sample_run)
    ReductionSingleton().load_set_options(reload, period)
    
def AppendDataFile(datafile, workspace=None):
    AssignSample(datafile)
    
def SetCentre(XVAL, YVAL):
    _printMessage('SetCentre(' + str(XVAL) + ',' + str(YVAL) + ')')
    SetBeamCenter(XVAL/1000.0, YVAL/1000.0)
    
def Correct():
    Reduce1D()
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
    
    # Get the bank we are looking at
    final_result = run_setup.getReducedWorkspace()
    CropWorkspace(sample_name, final_result,
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

    _applyMasking(final_result, SPECMIN, DIMENSION, orientation,True)
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
    
    ################################################## ################################
        
    ################################ Correction in Q space ############################
    # 1D
    if CORRECTION_TYPE == '1D':
        if finding_centre == True:
            GroupIntoQuadrants(tmpWS, final_result, maskpt_rmin[0], maskpt_rmin[1], Q_REBIN)
            return
        else:
            conv = RunQ1D()
            conv.execute(ReductionSingleton, tmpWS,final_result,final_result,Q_REBIN, GRAVITY)
    # 2D    
    else:
        if finding_centre == True:
            raise Exception('Running center finding in 2D analysis is not possible, set1D() first.') 
        # Run 2D algorithm
        conv = RunQxy()
        conv.execute(tmpWS, final_result, QXY2, DQXY)

    mantid.deleteWorkspace(tmpWS)
    return

def WavRangeReduction(wav_start = None, wav_end = None, use_def_trans = True, finding_centre = False):
    SetWavelengthRange(wav_start, wav_end)
    Correct()

def SetWavelengthRange(start, end):
    ReductionSingleton().set_wavelength_range(start, end)

def SetWorkspaceName(name):
    ReductionSingleton().set_workspace_name(name)


def SetSampleOffset(value):
    INSTRUMENT.set_sample_offset(value)

def SetRearEfficiencyFile(filename):
    global DIRECT_BEAM_FILE_R
    DIRECT_BEAM_FILE_R = filename

def SetFrontEfficiencyFile(filename):
    global DIRECT_BEAM_FILE_F
    DIRECT_BEAM_FILE_F = filename

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
    Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=GRAVITY)

    flag_value = -10.0
    ReplaceSpecialValues(InputWorkspace=output,OutputWorkspace=output,NaNValue=flag_value,InfinityValue=flag_value)
    if CORRECTION_TYPE == '1D':
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

def PlotResult(workspace):
    if CORRECTION_TYPE == '1D':
        plotSpectrum(workspace,0)
    else:
        qti.app.mantidUI.importMatrixWorkspace(workspace).plotGraph2D()

##################### View mask details #####################################################

def ViewCurrentMask():
    top_layer = 'CurrentMask'
    LoadEmptyInstrument(INSTR_DIR + '/' + INSTRUMENT.name() + "_Definition.xml",top_layer)
    if RMIN > 0.0: 
        SANSUtility.MaskInsideCylinder(top_layer, RMIN, XBEAM_CENTRE, YBEAM_CENTRE)
    if RMAX > 0.0:
        SANSUtility.MaskOutsideCylinder(top_layer, RMAX, 0.0, 0.0)

    dimension = SANSUtility.GetInstrumentDetails(INSTRUMENT)[0]

    #_applyMasking() must be called on both detectors, the detector is specified by passing the index of it's first spectrum
    #start with the currently selected detector
    firstSpec1 = INSTRUMENT.cur_detector().firstSpec
    _applyMasking(top_layer, firstSpec1, dimension, SANSUtility.Orientation.Horizontal, False, True)
    #now the other detector
    firstSpec2 = INSTRUMENT.otherDetector().firstSpec
    _applyMasking(top_layer, firstSpec2, dimension, SANSUtility.Orientation.Horizontal, False, False)
    
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