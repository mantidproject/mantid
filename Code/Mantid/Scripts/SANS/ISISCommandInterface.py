"""
    Enables the SANS commands (listed at http://www.mantidproject.org/SANS) to
    be run
"""
import os

import isis_instrument
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
import isis_reduction_steps
import isis_reducer
import MantidFramework

#remove the following
DEL__FINDING_CENTRE_ = False
# disable plotting if running outside Mantidplot
try:
    import mantidplot
except:
    #this should happen when this is called from outside Mantidplot and only then, the result is that attempting to plot will raise an exception
    pass

try:
    from PyQt4.QtGui import qApp
    def appwidgets():
        return qApp.allWidgets()
except ImportError:
    def appwidgets():
        return []

_VERBOSE_ = False

__ISIS_reducer_instance_ = isis_reducer.ISISReducer()

def ISIS_global():
    """
        Returns an isis_reducer object that is shared by all
        dictionaries, e.g. GUI and scripting
    """
    return __ISIS_reducer_instance_

def reset_singleton(new=None):
    """
        Creates a new isis_reducer which is shared by all
        dictionaries (GUI and scripting)
    """
    global __ISIS_reducer_instance_
    if new is None:
        __ISIS_reducer_instance_ = isis_reducer.ISISReducer()
    else:
        __ISIS_reducer_instance_ = new

def SetVerboseMode(state):
#TODO: this needs to be on the reducer
    _VERBOSE_ = state

# Print a message and log it if the 
def _printMessage(msg, log = True, no_console=False):
    if log == True and _VERBOSE_ == True:
        MantidFramework.mtd.sendLogMessage('::SANS::' + msg)
    if not no_console:
        print msg
    
def issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    isis_reduction_steps._issueWarning(msg)
                
def UserPath(path):
    _printMessage('UserPath("' + path + '") #Will look for mask file here')
    ISIS_global().user_file_path = path

def DataPath(path):
    ISIS_global().set_data_path(path)

def SANS2D():
    _printMessage('SANS2D()')
    instrument = isis_instrument.SANS2D()
        
    ISIS_global().set_instrument(instrument)

def LOQ():
    _printMessage('LOQ()')
    instrument = isis_instrument.LOQ()

    ISIS_global().set_instrument(instrument)
    
def Detector(det_name):
    _printMessage('Detector("' + det_name + '")')
    ISIS_global().instrument.setDetector(det_name)
    
def Mask(details):
    _printMessage('Mask("' + details + '")')
    ISIS_global().mask.parse_instruction(details)
    
def MaskFile(file_name):
    _printMessage('#Opening "'+file_name+'"')
    ISIS_global().user_settings = isis_reduction_steps.UserFile(
        file_name)
    status = ISIS_global().user_settings.execute(
        ISIS_global(), None)
    _printMessage('#Success reading "'+file_name+'"'+' is '+str(status))
    return status
    
def SetMonitorSpectrum(specNum, interp=False):
    ISIS_global().set_monitor_spectrum(specNum, interp)

def SuggestMonitorSpectrum(specNum, interp=False):  
    ISIS_global().suggest_monitor_spectrum(specNum, interp)
    
def SetTransSpectrum(specNum, interp=False):
    ISIS_global().set_trans_spectrum(specNum, interp)
      
def SetPhiLimit(phimin,phimax, phimirror=True):
    ISIS_global().set_phi_limit(phimin, phimax, phimirror)
    
def SetSampleOffset(value):
    ISIS_global().instrument.set_sample_offset(value)
    
def Gravity(flag):
    _printMessage('Gravity(' + str(flag) + ')')
    ISIS_global().to_Q.set_gravity(flag)
    
def TransFit(mode,lambdamin=None,lambdamax=None):
    message = str(mode)
    if lambdamin: message += str(lambdamin)
    if lambdamax: message += str(lambdamax)
    _printMessage("TransFit(\"" + message + "\")")

    ISIS_global().set_trans_fit(lambdamin, lambdamax, mode)

def AssignCan(can_run, reload = True, period = -1):
    _printMessage('AssignCan("' + can_run + '")')
    ISIS_global().set_background(can_run, reload = reload, period = period)

    return ISIS_global().background_subtracter.assign_can(
                                                    ISIS_global())

def TransmissionSample(sample, direct, reload = True, period_t = -1, period_d = -1):
    """
        Specify the transmission and direct runs for the sample
        @param sample: the transmission run
        @param direct: direct run
        @param reload: if to replace the workspace if it is already there
        @param period_t: the entry number of the transmission run (default single entry file)  
        @param period_d: the entry number of the direct run (default single entry file)
    """  
    _printMessage('TransmissionSample("' + sample + '","' + direct + '")')
    ISIS_global().set_trans_sample(sample, direct, True, period_t, period_d)
    return ISIS_global().samp_trans_load.execute(
                                        ISIS_global(), None)

def TransmissionCan(can, direct, reload = True, period_t = -1, period_d = -1):
    """
        Specify the transmission and direct runs for the can
        @param can: the transmission run
        @param direct: direct run
        @param reload: if to replace the workspace if it is already there
        @param period_t: the entry number of the transmission run (default single entry file)  
        @param period_d: the entry number of the direct run (default single entry file)
    """
    _printMessage('TransmissionCan("' + can + '","' + direct + '")')
    ISIS_global().set_trans_can(can, direct, True, period_t, period_d)
    return ISIS_global().can_trans_load.execute(
                                            ISIS_global(), None)
    
def AssignSample(sample_run, reload = True, period = -1):
    _printMessage('AssignSample("' + sample_run + '")')

    ISIS_global().data_loader = isis_reduction_steps.LoadSample(
                                    sample_run, reload, period)

    sample_wksp, logs = ISIS_global().data_loader.execute(
                                            ISIS_global(), None)
    ISIS_global().set_run_number(sample_wksp)
    return sample_wksp, logs

def SetCentre(XVAL, YVAL):
    _printMessage('SetCentre(' + str(XVAL) + ',' + str(YVAL) + ')')

    ISIS_global().set_beam_finder(sans_reduction_steps.BaseBeamFinder(float(XVAL)/1000.0, float(YVAL)/1000.0))


def GetMismatchedDetList():
    """
        Return the list of mismatched detector names
    """
    return ISIS_global().instrument.get_marked_dets()

def WavRangeReduction(wav_start = None, wav_end = None, full_trans_wav = None, no_clean=False):
    """
        Run a reduction that has been set up and reset the old
        setup (unless clean = False)
        @param wav_start: the first wavelength to be in the output data
        @param wav_end: the last wavelength in the output data
        @param full_trans_wav if to use a wide wavelength range for the transmission correction, true by default
        @param no_clean best to leave this at the default (false) as then all old settings are cleared
    """
    _printMessage('WavRangeReduction(' + str(wav_start) + ',' + str(wav_end) + ','+str(full_trans_wav)+')')

    if not full_trans_wav is None:
        ISIS_global().full_trans_wav = full_trans_wav

    ISIS_global().to_wavelen.set_range(wav_start, wav_end)
    _printMessage('Running reduction for ' + str(ISIS_global().to_wavelen))



    if DEL__FINDING_CENTRE_ == True:
        final_workspace = wsname_cache.split('_')[0] + '_quadrants'
        if reduction_chain.to_Q.output_type == '1D':
                GroupIntoQuadrants(tmpWS, final_result, maskpt_rmin[0], maskpt_rmin[1], Q_REBIN)
                return

    try:
        #run the chain that must already have been setup
        if ISIS_global().clean:
            #the normal situation
            result = ISIS_global()._reduce()
        else:
            #here the reduction chain has been run once before and only some parts need to be re-run
            start_from = ISIS_global().step_num(ISIS_global().out_name)
            #using this code you take a risk that some values will come over from previous reductions, the risk should be minimised 
            result = ISIS_global().run_steps(start_ind=start_from)
    finally:
        if not no_clean:
            #normal behaviour, the chain has been used, at least a bit reset it
            reset_singleton()
        else:
            #set the analysis to use the same run file again        
            workspace = ISIS_global().data_loader.uncropped
            ISIS_global()._data_files.clear()
            ISIS_global()._data_files[workspace] = workspace

    return result

    
def _SetWavelengthRange(start, end):
    ISIS_global().to_wavelen.set_range(start, end)


def Set1D():
    _printMessage('Set1D()')
    ISIS_global().to_Q.output_type = '1D'

def Set2D():
    _printMessage('Set2D()')
    ISIS_global().to_Q.output_type = '2D'

def SetRearEfficiencyFile(filename):
    rear_det = ISIS_global().instrument.getDetector('rear')
    rear_det.correction_file = filename

def SetFrontEfficiencyFile(filename):
    front_det = ISIS_global().instrument.getDetector('front')
    front_det.correction_file = filename

def SetDetectorFloodFile(filename):
    ISIS_global().flood_file.set_filename(filename)

def displayUserFile():
    print '-- Mask file defaults --'
    print ISIS_global().to_wavlen
    print ISIS_global().Q_string()
#    print correction_files()
    print '    direct beam file rear:',
    print ISIS_global().instrument.detector_file('rear')
    print '    direct beam file front:',
    print ISIS_global().instrument.detector_file('front')
    print ISIS_global().mask

def displayMaskFile():
    displayUserFile()

def displayGeometry():
    [x, y] = ISIS_global()._beam_finder.get_beam_center()
    print 'Beam centre: [' + str(x) + ',' + str(y) + ']'
    print ISIS_global().geometry

def SetPhiLimit(phimin,phimax, phimirror=True):
    maskStep = ISIS_global().get_mask()
    #a beam centre of [0,0,0] makes sense if the detector has been moved such that beam centre is at [0,0,0]
    maskStep.set_phi_limit(phimin, phimax, phimirror)
    
def LimitsPhi(phimin, phimax, use_mirror=True):
    '''
        !!DEPRECIATED by the function above, remove!!
        need to remove from SANSRunWindow.cpp
    '''
    settings = ISIS_global().user_settings
    if use_mirror :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=True)')
        settings.readLimitValues('L/PHI ' + str(phimin) + ' ' + str(phimax), ISIS_global())
    else :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=False)')
        settings.readLimitValues('L/PHI/NOMIRROR ' + str(phimin) + ' ' + str(phimax), ISIS_global())

def LimitsR(rmin, rmax):
    _printMessage('LimitsR(' + str(rmin) + ',' +str(rmax) + ')', ISIS_global())
    settings = ISIS_global().user_settings
    settings.readLimitValues('L/R ' + str(rmin) + ' ' + str(rmax) + ' 1', ISIS_global())

def LimitsWav(lmin, lmax, step, type):
    _printMessage('LimitsWav(' + str(lmin) + ',' + str(lmax) + ',' + str(step) + ','  + type + ')')
    settings = ISIS_global().user_settings
    settings.readLimitValues('L/WAV ' + str(lmin) + ' ' + str(lmax) + ' ' + str(step) + '/'  + type, ISIS_global())

def LimitsQ(*args):
    settings = ISIS_global().user_settings
    # If given one argument it must be a rebin string
    if len(args) == 1:
        val = args[0]
        if type(val) == str:
            _printMessage("LimitsQ(" + val + ")")
            settings.readLimitValues("L/Q " + val, ISIS_global())
        else:
            _issueWarning("LimitsQ can only be called with a single string or 4 values")
    elif len(args) == 4:
        qmin,qmax,step,step_type = args
        _printMessage('LimitsQ(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(step_type) + ')')
        settings.readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + step_type, ISIS_global())
    else:
        _issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")

def LimitsQXY(qmin, qmax, step, type):
    _printMessage('LimitsQXY(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(type) + ')')
    settings = ISIS_global().user_settings

    settings.readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type, ISIS_global())

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
    Q1D(output,rawcount_ws,output,q_bins,AccountForGravity=ISIS_global().to_Q.get_gravity())

    flag_value = -10.0
    ReplaceSpecialValues(InputWorkspace=output,OutputWorkspace=output,NaNValue=flag_value,InfinityValue=flag_value)
    if ISIS_global().to_Q.output_type == '1D':
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
            RESIDUE_GRAPH = mantidplot.plotSpectrum('Left', 0)
            mantidplot.mergePlots(RESIDUE_GRAPH, plotSpectrum(['Right','Up'],0))
            mantidplot.mergePlots(RESIDUE_GRAPH, plotSpectrum(['Down'],0))
        RESIDUE_GRAPH.activeLayer().setTitle("Itr " + str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))
    except:
        #if plotting is not available it probably means we are running outside a GUI, in which case do everything but don't plot
        pass
        
    mantid.sendLogMessage("::SANS::Itr: "+str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))              
    return residueX, residueY

def PlotResult(workspace):
    try:
        numSpecs = workspace.getNumberHistograms()
    except AttributeError:
        numSpecs = MantidFramework.mtd[workspace].getNumberHistograms()

    try:
        if numSpecs == 1:
            mantidplot.plotSpectrum(workspace,0)
        else:        
            mantidplot.importMatrixWorkspace(workspace).plotGraph2D()

    except NameError:
        issueWarning('Plot functions are not available, is this being run from outside Mantidplot?')

##################### View mask details #####################################################

def ViewCurrentMask():
    ISIS_global().ViewCurrentMask()

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
    script += '[COLETTE]  LIMIT/RADIUS ' + str(ISIS_global().mask.min_radius)
    script += ' ' + str(ISIS_global().mask.max_radius) + '\n'
    script += '[COLETTE]  LIMIT/WAVELENGTH ' + ISIS_global().to_wavelen.get_range() + '\n'
    if ISIS_global().DWAV <  0:
        script += '[COLETTE]  STEP/WAVELENGTH/LOGARITHMIC ' + str(ISIS_global().to_wavelen.w_step)[1:] + '\n'
    else:
        script += '[COLETTE]  STEP/WAVELENGTH/LINEAR ' + str(ISIS_global().to_wavelen.w_step) + '\n'
    # For the moment treat the rebin string as min/max/step
    qbins = ISIS_global().Q_REBEIN.split(",")
    nbins = len(qbins)
    if ISIS_global().to_Q.output_type == '1D':
        script += '[COLETTE]  LIMIT/Q ' + str(qbins[0]) + ' ' + str(qbins[nbins-1]) + '\n'
        dq = float(qbins[1])
        if dq <  0:
            script += '[COLETTE]  STEP/Q/LOGARITHMIC ' + str(dq)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/Q/LINEAR ' + str(dq) + '\n'
    else:
        script += '[COLETTE]  LIMIT/QXY ' + str(0.0) + ' ' + str(ISIS_global().QXY2) + '\n'
        if ISIS_global().DQXY <  0:
            script += '[COLETTE]  STEP/QXY/LOGARITHMIC ' + str(ISIS_global().DQXY)[1:] + '\n'
        else:
            script += '[COLETTE]  STEP/QXY/LINEAR ' + str(ISIS_global().DQXY) + '\n'
    
    # Correct
    script += '[COLETTE] CORRECT\n'
    if plotresults:
        script += '[COLETTE]  DISPLAY/HISTOGRAM ' + reduced + '\n'
    if savepath != '':
        script += '[COLETTE]  WRITE/LOQ ' + reduced + ' ' + savepath + '\n'
        
    return script

#this is like a #define I'd like to get rid of it because it means nothing here
DefaultTrans = 'True'
NewTrans = 'False'