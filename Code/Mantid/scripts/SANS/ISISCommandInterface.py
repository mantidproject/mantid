"""
    Enables the SANS commands (listed at http://www.mantidproject.org/SANS) to
    be run
"""
import isis_instrument
from reduction.command_interface import ReductionSingleton  
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
import isis_reduction_steps
import isis_reducer
import centre_finder as centre
import SANSReduction
import MantidFramework
import copy
from SANSadd2 import *

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
LAST_SAMPLE = None
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
    ReductionSingleton().user_file_path = path

def DataPath(path):
    ReductionSingleton().set_data_path(path)

def SANS2D():
    """
        Initialises the instrument settings for SANS2D()
    """
    _printMessage('SANS2D()')
    instrument = isis_instrument.SANS2D()
        
    ReductionSingleton().set_instrument(instrument)

def LOQ():
    _printMessage('LOQ()')
    instrument = isis_instrument.LOQ()

    ReductionSingleton().set_instrument(instrument)
    
def Detector(det_name):
    _printMessage('Detector("' + det_name + '")')
    ReductionSingleton().instrument.setDetector(det_name)
    
def Mask(details):
    _printMessage('Mask("' + details + '")')
    ReductionSingleton().mask.parse_instruction(details)
    
def MaskFile(file_name):
    _printMessage('#Opening "'+file_name+'"')
    ReductionSingleton().user_settings = isis_reduction_steps.UserFile(
        file_name)
    status = ReductionSingleton().user_settings.execute(
        ReductionSingleton(), None)
    _printMessage('#Success reading "'+file_name+'"'+' is '+str(status))
    return status
    
def SetMonitorSpectrum(specNum, interp=False):
    ReductionSingleton().set_monitor_spectrum(specNum, interp)

def SetTransSpectrum(specNum, interp=False):
    ReductionSingleton().set_trans_spectrum(specNum, interp)
      
def SetPhiLimit(phimin,phimax, phimirror=True):
    ReductionSingleton().set_phi_limit(phimin, phimax, phimirror)
    
def SetSampleOffset(value):
    ReductionSingleton().instrument.set_sample_offset(value)
    
def Gravity(flag):
    _printMessage('Gravity(' + str(flag) + ')')
    ReductionSingleton().to_Q.set_gravity(flag)
    
def TransFit(mode,lambdamin=None,lambdamax=None):
    mode = str(mode).strip().upper()
    message = mode
    if lambdamin: message += str(lambdamin)
    if lambdamax: message += str(lambdamax)
    _printMessage("TransFit(\"" + message + "\")")

    ReductionSingleton().set_trans_fit(lambdamin, lambdamax, mode)
    
def TransWorkspace(sample, can = None):
    """
        Use a given workpspace that contains pre-calculated transmissions
        @param sample the workspace to use for the sample
        @param can calculated transmission for the can 
    """
    ReductionSingleton().transmission_calculator.calculated_samp = sample 
    ReductionSingleton().transmission_calculator.calculated_can = can 

def AssignCan(can_run, reload = True, period = -1):
    _printMessage('AssignCan("' + can_run + '")')
    ReductionSingleton().set_background(can_run, reload = reload, period = period)

    logs = ReductionSingleton().background_subtracter.assign_can(
          ReductionSingleton())
    return ReductionSingleton().background_subtracter.workspace.wksp_name, logs

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
    ReductionSingleton().set_trans_sample(sample, direct, True, period_t, period_d)
    return ReductionSingleton().samp_trans_load.execute(
                                        ReductionSingleton(), None)

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
    ReductionSingleton().set_trans_can(can, direct, True, period_t, period_d)
    return ReductionSingleton().can_trans_load.execute(
                                            ReductionSingleton(), None)
    
def AssignSample(sample_run, reload = True, period = -1):
    _printMessage('AssignSample("' + sample_run + '")')

    ReductionSingleton().data_loader = isis_reduction_steps.LoadSample(
                                    sample_run, reload, period)

    logs = ReductionSingleton().data_loader.execute(
                                            ReductionSingleton(), None)
    
    global LAST_SAMPLE
    LAST_SAMPLE = ReductionSingleton().sample_wksp 
    return ReductionSingleton().sample_wksp, logs

def SetCentre(xcoord, ycoord):
    _printMessage('SetCentre(' + str(xcoord) + ',' + str(ycoord) + ')')

    ReductionSingleton().set_beam_finder(sans_reduction_steps.BaseBeamFinder(
                                float(xcoord)/1000.0, float(ycoord)/1000.0))

def GetMismatchedDetList():
    """
        Return the list of mismatched detector names
    """
    return ReductionSingleton().instrument.get_marked_dets()

def WavRangeReduction(wav_start=None, wav_end=None, full_trans_wav=None, name_suffix=None):
    """
        Run a reduction that has been set up, from loading the raw data to calculating Q,  and
        reset the old setup
        @param wav_start: the first wavelength to be in the output data
        @param wav_end: the last wavelength in the output data
        @param full_trans_wav: if to use a wide wavelength range (whole range specified by L/WAV) for the transmission correction, true by default
    """
    _printMessage('WavRangeReduction(' + str(wav_start) + ',' + str(wav_end) + ','+str(full_trans_wav)+')')

    if not full_trans_wav is None:
        ReductionSingleton().full_trans_wav = full_trans_wav

    ReductionSingleton().to_wavelen.set_range(wav_start, wav_end)
    _printMessage('Running reduction for ' + str(ReductionSingleton().to_wavelen))

    result = Reduce()
    if name_suffix:
        old = result
        result += name_suffix
        RenameWorkspace(old, result)
        
    return result

def CompWavRanges(wavelens, plot=True):
    """
        Compares the momentum transfer results calculated from different wavelength ranges. Given
        the list of wave ranges [a, b, c] it reduces for wavelengths a-b, b-c and a-c.
        @param wavelens: the list of wavelength ranges
        @param plot: set this to true to plot the result (must be run in Mantid), default is true
    """ 

    _printMessage('CompWavRanges( %s,plot=%s)'%(str(wavelens),plot))

    #this only makes sense for 1D reductions
    if ReductionSingleton().to_Q.output_type == '2D':
        issueWarning('This wave ranges check is a 1D analysis, ignoring 2D setting')
        _printMessage('Set1D()')
        ReductionSingleton().to_Q.output_type = '1D'
    
    if type(wavelens) != type([]) or len(wavelens) < 2:
        if type(wavelens) != type((1,)):
            raise RuntimeError('Error CompWavRanges() requires a list of wavelengths between which reductions will be performed.')
    
    try:
        ReductionSingleton().to_wavelen.set_rebin(w_low=wavelens[0],
            w_high=wavelens[len(wavelens)-1])
        #run the reduction calculated will be an array with the names of all the workspaces produced
        calculated = [ReductionSingleton()._reduce()]
        for i in range(0, len(wavelens)-1):
            settings = copy.deepcopy(ReductionSingleton().reference())
            ReductionSingleton().to_wavelen.set_rebin(
                            w_low=wavelens[i], w_high=wavelens[i+1])
            calculated.append(ReductionSingleton().run_from_raw())
            ReductionSingleton().replace(settings)
    finally:
        ReductionSingleton.clean(isis_reducer.ISISReducer)

    if plot:
        mantidplot.plotSpectrum(calculated, 0)
    
    #return just the workspace name of the full range
    return calculated[0]

def Reduce():
    try:
        result = ReductionSingleton()._reduce()
    finally:
        ReductionSingleton.clean(isis_reducer.ISISReducer)

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

def SetDetectorFloodFile(filename):
    ReductionSingleton().flood_file.set_filename(filename)

def displayUserFile():
    print '-- Mask file defaults --'
    print ReductionSingleton().to_wavlen
    print ReductionSingleton().Q_string()
#    print correction_files()
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
    
def LimitsPhi(phimin, phimax, use_mirror=True):
    '''
        !!DEPRECIATED by the function above, remove!!
        need to remove from SANSRunWindow.cpp
    '''
    if use_mirror :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=True)')
        ReductionSingleton().mask.set_phi_limit(phimin, phimax, True)
    else :
        _printMessage("LimitsPHI(" + str(phimin) + ' ' + str(phimax) + 'use_mirror=False)')
        ReductionSingleton().mask.set_phi_limit(phimin, phimax, False)

def LimitsR(rmin, rmax, quiet=False):
    if not quiet:
        _printMessage('LimitsR(' + str(rmin) + ',' +str(rmax) + ')', ReductionSingleton())

    ReductionSingleton().mask.set_radi(rmin, rmax)
    ReductionSingleton().CENT_FIND_RMIN = float(rmin)/1000.
    ReductionSingleton().CENT_FIND_RMAX = float(rmax)/1000.    

def LimitsWav(lmin, lmax, step, bin_type):
    _printMessage('LimitsWav(' + str(lmin) + ',' + str(lmax) + ',' + str(step) + ','  + bin_type + ')')
    
    if ( bin_type.upper().strip() == 'LINEAR'): bin_type = 'LIN'
    if ( bin_type.upper().strip() == 'LOGARITHMIC'): bin_type = 'LOG'
    if bin_type == 'LOG':
        bin_sym = '-'
    else:
        bin_sym = ''
    
    ReductionSingleton().to_wavelen.set_rebin(lmin, bin_sym + str(step), lmax)

def LimitsQ(*args):
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError('MaskFile() first')

    # If given one argument it must be a rebin string
    if len(args) == 1:
        val = args[0]
        if type(val) == str:
            _printMessage("LimitsQ(" + val + ")")
            settings.readLimitValues("L/Q " + val, ReductionSingleton())
        else:
            issueWarning("LimitsQ can only be called with a single string or 4 values")
    elif len(args) == 4:
        qmin,qmax,step,step_type = args
        _printMessage('LimitsQ(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(step_type) + ')')
        settings.readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + step_type, ReductionSingleton())
    else:
        issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")

def LimitsQXY(qmin, qmax, step, type):
    _printMessage('LimitsQXY(' + str(qmin) + ',' + str(qmax) +',' + str(step) + ',' + str(type) + ')')
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError('MaskFile() first')

    settings.readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type, ReductionSingleton())

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
            RESIDUE_GRAPH = mantidplot.plotSpectrum('Left', 0)
            mantidplot.mergePlots(RESIDUE_GRAPH, plotSpectrum(['Right','Up'],0))
            mantidplot.mergePlots(RESIDUE_GRAPH, plotSpectrum(['Down'],0))
        RESIDUE_GRAPH.activeLayer().setTitle("Itr " + str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))
    except:
        #if plotting is not available it probably means we are running outside a GUI, in which case do everything but don't plot
        pass
        
    mantid.sendLogMessage("::SANS::Itr: "+str(ITER_NUM)+" "+str(XVAR_PREV*1000.)+","+str(YVAR_PREV*1000.)+" SX "+str(residueX)+" SY "+str(residueY))              
    return residueX, residueY

def PlotResult(workspace, canvas=None):
    """
        Draws a graph of the passed workspace. If the workspace is 2D (has many spectra
        a contour plot is written
        @param workspace: a workspace name or handle to plot
        @param canvas: optional handle to an existing graph to write the plot to
        @return: a handle to the graph that was written to
    """ 
    try:
        numSpecs = workspace.getNumberHistograms()
    except AttributeError:
        #ensure that we are dealing with a workspace handle rather than its name
        workspace = MantidFramework.mtd[workspace]
        numSpecs = workspace.getNumberHistograms()

    try:
        if numSpecs == 1:
            graph = mantidplot.plotSpectrum(workspace,0)
        else:        
            graph = mantidplot.importMatrixWorkspace(workspace.getName()).plotGraph2D()

    except NameError:
        issueWarning('Plot functions are not available, is this being run from outside Mantidplot?')
        
    if not canvas is None:
        #we were given a handle to an existing graph, use it
        mantidplot.mergePlots(canvas, graph)
        graph = canvas
    
    return graph

##################### View mask details #####################################################

def ViewCurrentMask():
    ReductionSingleton().ViewCurrentMask()

def DisplayMask(mask_worksp=None):
    """
        Displays masking by applying it to a workspace and displaying
        it in instrument view. If no workspace is passed a copy of the
        sample workspace is used, unless no sample was loaded and then
        an empty instrument will be shown
        @param mask_worksp: optional this named workspace will be modified and should be from the currently selected instrument
        @return the name of the workspace that was displayed
    """
    #this will be copied from a sample work space if one exists
    counts_data = None
    instrument = ReductionSingleton().instrument
    
    if not mask_worksp:
        mask_worksp = '__CurrentMask'
        samp = ReductionSingleton().sample_wksp
        global LAST_SAMPLE
        samp = LAST_SAMPLE 
        
        if samp:
            counts_data = '__DisplayMasked_tempory_wksp'
            Integration(samp, counts_data)
            CloneWorkspace(samp, mask_worksp)
        else:
            instrument.load_empty(mask_worksp)
        
    ReductionSingleton().mask.display(mask_worksp, instrument, counts_data)
    if counts_data:
        mantid.deleteWorkspace(counts_data)
        
    return mask_worksp

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
    if ReductionSingleton().DWAV <  0:
        script += '[COLETTE]  STEP/WAVELENGTH/LOGARITHMIC ' + str(ReductionSingleton().to_wavelen.w_step)[1:] + '\n'
    else:
        script += '[COLETTE]  STEP/WAVELENGTH/LINEAR ' + str(ReductionSingleton().to_wavelen.w_step) + '\n'
    # For the moment treat the rebin string as min/max/step
    qbins = ReductionSingleton().Q_REBEIN.split(",")
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

def FindBeamCentre(rlow, rupp, MaxIter = 10, x_start = None, y_start = None):
    """
        Assumes that the run is setup, sample assigned etc
    """
    sample_ws = SANSReduction.AssignSample(run_file)[0]
    if (not sample_ws) or (len(sample_ws) == 0):
        issueWarning('Cannot load sample run "' + run_file + '", skipping reduction')
        return
    
    #Sample trans
    run_file = run['sample_trans']
    run_file2 = run['sample_direct_beam']
    ws1, ws2 = TransmissionSample(run_file + format, run_file2 + format)
    if len(run_file) > 0 and len(ws1) == 0:
        issueWarning('Cannot load trans sample run "' + run_file + '", skipping reduction')
        return
    if len(run_file2) > 0 and len(ws2) == 0: 
        issueWarning('Cannot load trans direct run "' + run_file2 + '", skipping reduction')
        return
    
    # Sans Can 
    run_file = run['can_sans']
    can_ws = AssignCan(run_file + format)[0]
    if run_file != '' and len(can_ws) == 0:
        issueWarning('Cannot load can run "' + run_file + '", skipping reduction')
        return

    #Can trans
    run_file = run['can_trans']
    run_file2 = run['can_direct_beam']
    ws1, ws2 = TransmissionCan(run_file + format, run_file2 + format)
    
def NewFindBeamCentre(rlow, rupp, MaxIter = 10, x_start = None, y_start = None):
    XSTEP = YSTEP = ReductionSingleton().cen_find_step

    setting = copy.deepcopy(ReductionSingleton().reference())

    LimitsR(str(float(rlow)/1000.), str(float(rupp)/1000.), quiet=True)

    if ( not x_start is None ) or ( not y_start is None):
        ReductionSingleton().set_beam_finder(
            sans_reduction_steps.BaseBeamFinder(
            float(x_start), float(y_start)))

    beamcoords = ReductionSingleton()._beam_finder.get_beam_center()
    XNEW = beamcoords[0]
    YNEW = beamcoords[1]

    mantid.sendLogMessage("::SANS:: xstart,ystart="+str(XNEW*1000.)+" "+str(YNEW*1000.)) 
    _printMessage("Starting centre finding routine ...")

    #make copies of the workspaces and the reduction chain object
    samp = ReductionSingleton().sample_wksp + '_cen'
    CloneWorkspace(ReductionSingleton().sample_wksp, workspace)
    if ReductionSingleton().background_subtracter:
        can = reducer.background_subtracter.workspace.wksp_name+'cen'
        CloneWorkspace(reducer.background_subtracter.workspace.wksp_name, can)
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    centre_reduction.sample_wksp = samp
    centre_reduction.background_subtracter.workspace.wksp_name = can

    #this function moves the detector to the beam center positions defined above and returns an estimate of where the beam center is relative to the new center  
    oldX2,oldY2 = centre.SeekCentre([XNEW, YNEW], centre_reduction)
    # take first trial step
    XNEW = x_start + XSTEP
    YNEW = y_start + YSTEP

    for ITER_NUM in range(1, MaxIter+1):
        _printMessage("Iteration " + str(ITER_NUM) + ": " + str(XNEW*1000.)+ "  "+ str(YNEW*1000.))

        ReductionSingleton().set_beam_finder(
            sans_reduction_steps.BaseBeamFinder(XNEW, YNEW))

        newX2,newY2 = centre.SeekCentre([XNEW, YNEW], centre_reduction, samp, can)
        
        #have we stepped across the y-axis that goes through the beam center?  
        if newX2 > oldX2:
            # yes with stepped across the middle, reverse direction and half the step size 
            XSTEP = -XSTEP/2.
        if newY2 > oldY2:
            YSTEP = -YSTEP/2.
        if abs(XSTEP) < 0.1251/1000. and abs(YSTEP) < 0.1251/1000. :
            # this is the success criteria, we've close enough to the center
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

    
    ReductionSingleton().set_beam_finder(
        sans_reduction_steps.BaseBeamFinder(XNEW, YNEW))
    _printMessage("Centre coordinates updated: [" + str(XNEW)+ ","+ str(YNEW) + ']')
    

    
    # Reload the sample and can
#    try to get rid of this by leaving the original loaded workspaces unchanged
    _assignHelper(_SAMPLE_RUN, False, PERIOD_NOS["SCATTER_SAMPLE"])
    if _CAN_RUN != '':
        _assignHelper(_CAN_RUN, False, PERIOD_NOS["SCATTER_CAN"])
        global _CAN_SETUP
        _CAN_SETUP = None


#this is like a #define I'd like to get rid of it because it seems meaningless here
DefaultTrans = 'True'
NewTrans = 'False'

ReductionSingleton.clean(isis_reducer.ISISReducer)


#if __name__ != '__main__':
#    AssignSample('c:\\mantid\\test\\data\\SANS2D\\SANS2D000992.raw')