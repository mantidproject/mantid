#pylint: disable=invalid-name
"""
    Enables the SANS commands (listed at http://www.mantidproject.org/SANS) to
    be run
"""
import isis_instrument
from reducer_singleton import ReductionSingleton
from mantid.kernel import Logger
sanslog = Logger("SANS")

import isis_reduction_steps
import isis_reducer
from centre_finder import CentreFinder as CentreFinder
#import SANSReduction
from mantid.simpleapi import *
from mantid.api import WorkspaceGroup
import copy
from SANSadd2 import *
import SANSUtility as su
from SANSUtility import deprecated

# disable plotting if running outside Mantidplot
try:
    import mantidplot
except:
    mantidplot = None
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
        sanslog.notice(msg)
    if not no_console:
        print msg

def issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    isis_reduction_steps._issueWarning(msg)

def _refresh_singleton():
    ReductionSingleton.clean(isis_reducer.ISISReducer)
    ReductionSingleton().remove_settings()

def Clean():
    """
    An exposed command to allow cleaning of the reducer, and any related
    settings.
    """
    _refresh_singleton()

def SANS2D(idf_path=None):
    """
        Initialises the instrument settings for SANS2D
        @param idf_path :: optionally specify the path to the SANS2D IDF to use.
                           Uses default if none specified.
        @return True on success
    """
    _printMessage('SANS2D()')
    try:
        instrument = isis_instrument.SANS2D(idf_path)

        ReductionSingleton().set_instrument(instrument)
        config['default.instrument']='SANS2D'
    except:
        return False
    return True

def SANS2DTUBES():
    """
    Quick, temporary workaround for the IDF problem we're fixing in #9367.
    Simply pass the correct IDF to SANS2D().
    """
    return SANS2D("SANS2D_Definition_Tubes.xml")

def LOQ():
    """
        Initialises the instrument settings for LOQ
        @return True on success
    """
    _printMessage('LOQ()')
    try:
        instrument = isis_instrument.LOQ()

        ReductionSingleton().set_instrument(instrument)
        config['default.instrument']='LOQ'
    except:
        return False
    return True

def LARMOR():
    """
    Initialises the instrument settings for LARMOR
    @return True on success
    """
    _printMessage('LARMOR()')
    try:
        instrument = isis_instrument.LARMOR()

        ReductionSingleton().set_instrument(instrument)
        config['default.instrument']='LARMOR'
    except:
        return False
    return True

def Detector(det_name):
    """
        Sets the detector bank to use for the reduction e.g. 'front-detector'. The
        main detector is assumed if this line is not given
        @param det_name: the detector's name
    """
    _printMessage('Detector("' + det_name + '")')
    ReductionSingleton().instrument.setDetector(det_name)

def Mask(details):
    """
        Specify regions of the detector to mask using the same syntax
        as used in the user file
        @param details: a string that specifies masking as it would appear in a mask file
    """
    _printMessage('Mask("' + details + '")')
    ReductionSingleton().mask.parse_instruction(ReductionSingleton().instrument.name(),details)

def MaskFile(file_name):
    """
        Loads the settings file. The settings are loaded as soon as this line is encountered
        and are overridden by other Python commands
        @param file_name: the settings file
    """
    _printMessage('#Opening "'+file_name+'"')

    # ensure that no slice string is kept from previous executions.
    ReductionSingleton().setSlicesLimits("")

    ReductionSingleton().user_settings = isis_reduction_steps.UserFile(
        file_name)
    status = ReductionSingleton().user_settings.execute(
        ReductionSingleton(), None)
    _printMessage('#Success reading "'+file_name+'"'+' is '+str(status))
    return status

def SetMonitorSpectrum(specNum, interp=False):
    """
        Specifies the spectrum number of the spectrum that will be used to
        for monitor normalisation
        @param specNum: a spectrum number (1 or greater)
        @param interp: when rebinning the wavelength bins to match the main workspace, if use interpolation default no interpolation
    """
    ReductionSingleton().set_monitor_spectrum(specNum, interp)

def SetTransSpectrum(specNum, interp=False):
    ReductionSingleton().set_trans_spectrum(specNum, interp)

def SetSampleOffset(value):
    ReductionSingleton().instrument.set_sample_offset(value)

def Gravity(flag):
    _printMessage('Gravity(' + str(flag) + ')')
    ReductionSingleton().to_Q.set_gravity(flag)

def SetFrontDetRescaleShift(scale=1.0, shift=0.0, fitScale=False, fitShift=False, qMin=None, qMax=None):
    """
        Stores property about the detector which is used to rescale and shift
        data in the bank after data have been reduced
        @param scale: Default to 1.0. Value to multiply data with
        @param shift: Default to 0.0. Value to add to data
        @param fitScale: Default is False. Whether or not to try and fit this param
        @param fitShift: Default is False. Whether or not to try and fit this param
        @param qMin: When set to None (default) then for fitting use the overlapping q region of front and rear detectors
        @param qMax: When set to None (default) then for fitting use the overlapping q region of front and rear detectors
    """
    ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift = ReductionSingleton().instrument.getDetector('FRONT')._RescaleAndShift(
        scale, shift, fitScale, fitShift, qMin, qMax)
    _printMessage('#Set front detector rescale/shift values')

def TransFit(mode,lambdamin=None,lambdamax=None, selector='BOTH'):
    """
        Sets the fit method to calculate the transmission fit and the wavelength range
        over which to do the fit. These arguments are passed to the algorithm
        CalculateTransmission. If mode is set to 'Off' then the unfitted workspace is
        used and lambdamin and max have no effect
        @param mode: can be 'Logarithmic' ('YLOG', 'LOG') 'OFF' ('CLEAR') or 'LINEAR' (STRAIGHT', LIN'), 'POLYNOMIAL2', 'POLYNOMIAL3', ...
        @param lambdamin: the lowest wavelength to use in any fit
        @param lambdamax: the end of the fit range
        @param selector: define for which transmission this fit specification is valid (BOTH, SAMPLE, CAN)
    """
    mode = str(mode).strip().upper()
    message = mode
    if lambdamin: message += ', ' + str(lambdamin)
    if lambdamax: message += ', ' + str(lambdamax)
    message += ', selector=' + selector
    _printMessage("TransFit(\"" + message + "\")")

    ReductionSingleton().set_trans_fit(lambdamin, lambdamax, mode, selector)

def TransWorkspace(sample, can = None):
    """
        Use a given workpspace that contains pre-calculated transmissions
        @param sample the workspace to use for the sample
        @param can calculated transmission for the can
    """
    ReductionSingleton().transmission_calculator.calculated_samp = sample
    ReductionSingleton().transmission_calculator.calculated_can = can

def _return_old_compatibility_assign_methods(ws_name):
    """For backward compatibility, AssignCan and AssignSample returns a tuple
    with workspace name and the log entry if available.

    In the future, those methods should return just workspace name
    """
    logs = ""
    if isinstance(ReductionSingleton().instrument, isis_instrument.SANS2D):
        try:
            logs = ReductionSingleton().instrument.get_detector_log(ws_name)
        except:
            pass
    return ws_name, logs

def AssignCan(can_run, reload = True, period = isis_reduction_steps.LoadRun.UNSET_PERIOD):
    """
        The can is a scattering run under the same conditions as the experimental run but the
        only the sample container is in the sample position. Hence allowing the effect of the
        container to be removed. The run is specified using instrumentrunnumber.extension,
        e.g. SANS2D7777.nxs. On calling this function the run is loaded to a workspace and the
        detector banks and other components moved as applicable. Currently only reload=true is
        supported.
        @param can_run: run number to analysis e.g. SANS2D7777.nxs
        @param reload: must be set to True
        @param period: the period (entry) number to load, default is the first period
    """
    mes = 'AssignCan("' + str(can_run) + '"'
    if period != isis_reduction_steps.LoadRun.UNSET_PERIOD:
        mes += ', ' + str(period)
    mes += ')'
    _printMessage(mes)

    ReductionSingleton().set_can(can_run, reload, period)
    return _return_old_compatibility_assign_methods(
        ReductionSingleton().get_can().wksp_name)

def TransmissionSample(sample, direct, reload = True, period_t = -1, period_d = -1):
    """
        Specify the transmission and direct runs for the sample
        @param sample: the transmission run
        @param direct: direct run
        @param reload: if to replace the workspace if it is already there
        @param period_t: the entry number of the transmission run (default single entry file)
        @param period_d: the entry number of the direct run (default single entry file)
    """
    _printMessage('TransmissionSample("' + str(sample) + '","' + str(direct) + '")')
    ReductionSingleton().set_trans_sample(sample, direct, reload, period_t, period_d)
    return ReductionSingleton().samp_trans_load.execute(\
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
    _printMessage('TransmissionCan("' + str(can) + '","' + str(direct) + '")')
    ReductionSingleton().set_trans_can(can, direct, reload, period_t, period_d)
    return ReductionSingleton().can_trans_load.execute(\
                                            ReductionSingleton(), None)

def AssignSample(sample_run, reload = True, period = isis_reduction_steps.LoadRun.UNSET_PERIOD):
    """
        Specifies the run to analyse using the format instrumentrunnumber.extension,
        e.g. SANS2D7777.nxs. This is one of the few commands that executes Mantid algorithms
        when called. Currently only reload=true is supported.
        @param sample_run: run number to analysis e.g. SANS2D7777.nxs
        @param reload: must be set to True
        @param period: the period (entry) number to load, default is the first period
    """
    mes = 'AssignSample("' + str(sample_run) + '"'
    if period != isis_reduction_steps.LoadRun.UNSET_PERIOD:
        mes += ', ' + str(period)
    mes += ')'
    _printMessage(mes)

    ReductionSingleton().set_sample(sample_run, reload, period)

    global LAST_SAMPLE
    LAST_SAMPLE = ReductionSingleton().get_sample().wksp_name
    return _return_old_compatibility_assign_methods(LAST_SAMPLE)

def SetCentre(xcoord, ycoord, bank = 'rear'):
    """
    Configure the Beam Center position. It support the configuration of the centre for
    the both detectors bank (low-angle bank and high-angle bank detectors)

    It allows defining the position for both detector banks.
    :param xcoord: X position of beam center in the user coordinate system.
    :param ycoord: Y position of beam center in the user coordinate system.
    :param bank: The selected bank ('rear' - low angle or 'front' - high angle)
    Introduced #5942
    """
    _printMessage('SetCentre(' + str(xcoord) + ', ' + str(ycoord) + ')')
    # use the scale factors from the parameter file to scale correctly
    XSF = ReductionSingleton().inst.beam_centre_scale_factor1
    YSF = ReductionSingleton().inst.beam_centre_scale_factor2

    ReductionSingleton().set_beam_finder(isis_reduction_steps.BaseBeamFinder(\
                                float(xcoord)/XSF, float(ycoord)/YSF), bank)

def GetMismatchedDetList():
    """
        Return the list of mismatched detector names
    """
    return ReductionSingleton().instrument.get_marked_dets()

def WavRangeReduction(wav_start=None, wav_end=None, full_trans_wav=None, name_suffix=None, combineDet=None, resetSetup=True, out_fit_settings = dict()):
    """
        Run reduction from loading the raw data to calculating Q. Its optional arguments allows specifics
        details to be adjusted, and optionally the old setup is reset at the end. Note if FIT of RESCALE or SHIFT
        is selected then both REAR and FRONT detectors are both reduced EXCEPT if only the REAR detector is selected
        to be reduced

        @param wav_start: the first wavelength to be in the output data
        @param wav_end: the last wavelength in the output data
        @param full_trans_wav: if to use a wide wavelength range, the instrument's default wavelength range, for the transmission correction, false by default
        @param name_suffix: append the created output workspace with this
        @param combineDet: combineDet can be one of the following:
                           'rear'                (run one reduction for the 'rear' detector data)
                           'front'               (run one reduction for the 'front' detector data, and rescale+shift 'front' data)
                           'both'                (run both the above two reductions)
                           'merged'              (run the same reductions as 'both' and additionally create a merged data workspace)
                            None                 (run one reduction for whatever detector has been set as the current detector
                                                  before running this method. If front apply rescale+shift)
        @param resetSetup: if true reset setup at the end
        @param out_fit_settings: An output parameter. It is used, specially when resetSetup is True, in order to remember the 'scale and fit' of the fitting algorithm.
        @return Name of one of the workspaces created
    """
    _printMessage('WavRangeReduction(' + str(wav_start) + ', ' + str(wav_end) + ', '+str(full_trans_wav)+')')
    # these flags indicate if it is necessary to reduce the front bank, the rear bank and if it is supposed to merge them
    reduce_rear_flag = False
    reduce_front_flag = False
    merge_flag = False

    retWSname_rear, retWSname_front, retWSname_merged = ["", "", ""]

    # combineDet from None to 'rear' or 'front'
    if combineDet is None:
        if ReductionSingleton().instrument.cur_detector().isAlias('FRONT'):
            combineDet = 'front'
        else:
            combineDet = 'rear'

    if not full_trans_wav is None:
        ReductionSingleton().full_trans_wav = full_trans_wav

    ReductionSingleton().to_wavelen.set_range(wav_start, wav_end)

    rAnds = ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift
    # check if fit is required.
    fitRequired = False
    if rAnds.fitScale or rAnds.fitShift:
        fitRequired = True

    com_det_option = combineDet.lower()

    # the only special case where reduce rear is not required is
    # if the user chose to reduce front and does not require fit
    if not (com_det_option == 'front' and not fitRequired):
        reduce_rear_flag = True
    if com_det_option != 'rear':
        reduce_front_flag = True
    if com_det_option == 'merged':
        merge_flag = True

    #The shift and scale is always on the front detector.
    if not reduce_front_flag:
        fitRequired = False

    #To backup value of singleton which are temporarily modified in this method
    toRestoreAfterAnalysis = ReductionSingleton().instrument.cur_detector().name()
    toRestoreOutputParts = ReductionSingleton().to_Q.outputParts

    # if 'merged' then when cross section is calculated output the two individual parts
    # of the cross section. These additional outputs are required to calculate
    # the merged workspace
    if merge_flag:
        ReductionSingleton().to_Q.outputParts = True

    # do reduce rear bank data
    if reduce_rear_flag:
        ReductionSingleton().instrument.setDetector('rear')
        retWSname_rear = _WavRangeReduction(name_suffix)
        retWSname = retWSname_rear

    # do reduce front bank
    if reduce_front_flag:
        # it is necessary to replace the Singleton if a reduction was done before
        if reduce_rear_flag:
            # In this case, it is necessary to reload the files, in order to move the components to the
            # correct position defined by its get_beam_center. (ticket #5942)

            # first copy the settings
            ReductionSingleton.replace(ReductionSingleton().cur_settings())

            # for the LOQ instrument, if the beam centers are different, we have to reload the data.
            if ReductionSingleton().instrument._NAME == 'LOQ' and\
                ReductionSingleton().get_beam_center('rear') != ReductionSingleton().get_beam_center('front'):

                # It is necessary to reload sample, transmission and can files.
                #reload sample
                issueWarning('Trying to reload workspaces')
                ReductionSingleton().instrument.setDetector('front')
                ReductionSingleton()._sample_run.reload(ReductionSingleton())
                #reassign can
                if ReductionSingleton().get_can():
                    ReductionSingleton().get_can().reload(ReductionSingleton())
                if ReductionSingleton().samp_trans_load:
                    #refresh Transmission
                    ReductionSingleton().samp_trans_load.execute(ReductionSingleton(), None)
                if ReductionSingleton().can_trans_load:
                    ReductionSingleton().can_trans_load.execute(ReductionSingleton(),None)

        ReductionSingleton().instrument.setDetector('front')

        retWSname_front = _WavRangeReduction(name_suffix)
        retWSname = retWSname_front

    # do fit and scale if required
    if fitRequired:
        scale, shift = _fitRescaleAndShift(rAnds, retWSname_front, retWSname_rear)
        ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift = shift
        ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale = scale
        if scale < 0:
            issueWarning("Fit returned SCALE negative")

    shift = ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift
    scale = ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale

    # apply the merge algorithm
    if merge_flag:
        retWSname_merged = retWSname_rear
        if retWSname_merged.count('rear') == 1:
            retWSname_merged = retWSname_merged.replace('rear', 'merged')
        else:
            retWSname_merged = retWSname_merged + "_merged"

        Nf = mtd[retWSname_front+"_sumOfNormFactors"]
        Nr = mtd[retWSname_rear+"_sumOfNormFactors"]
        Cf = mtd[retWSname_front+"_sumOfCounts"]
        Cr = mtd[retWSname_rear+"_sumOfCounts"]
        consider_can = True
        try:
            Nf_can = mtd[retWSname_front+"_can_tmp_sumOfNormFactors"]
            Nr_can = mtd[retWSname_rear+"_can_tmp_sumOfNormFactors"]
            Cf_can = mtd[retWSname_front+"_can_tmp_sumOfCounts"]
            Cr_can = mtd[retWSname_rear+"_can_tmp_sumOfCounts"]
            if Cr_can is None:
                consider_can = False
        except KeyError :
            #The CAN was not specified
            consider_can = False


        fisF = mtd[retWSname_front]
        fisR = mtd[retWSname_rear]

        minQ = min(min(fisF.dataX(0)), min(fisR.dataX(0)))
        maxQ = max(max(fisF.dataX(0)), max(fisR.dataX(0)))

        if maxQ > minQ:
            #preparing the sample
            Nf = CropWorkspace(InputWorkspace=Nf, OutputWorkspace=Nf, XMin=minQ, XMax=maxQ)
            Nr = CropWorkspace(InputWorkspace=Nr, OutputWorkspace=Nr, XMin=minQ, XMax=maxQ)
            Cf = CropWorkspace(InputWorkspace=Cf, OutputWorkspace=Cf, XMin=minQ, XMax=maxQ)
            Cr = CropWorkspace(InputWorkspace=Cr, OutputWorkspace=Cr, XMin=minQ, XMax=maxQ)
            if consider_can:
                #preparing the can
                Nf_can = CropWorkspace(InputWorkspace=Nf_can, OutputWorkspace=Nf_can, XMin=minQ, XMax=maxQ)
                Nr_can = CropWorkspace(InputWorkspace=Nr_can, OutputWorkspace=Nr_can, XMin=minQ, XMax=maxQ)
                Cf_can = CropWorkspace(InputWorkspace=Cf_can, OutputWorkspace=Cf_can, XMin=minQ, XMax=maxQ)
                Cr_can = CropWorkspace(InputWorkspace=Cr_can, OutputWorkspace=Cr_can, XMin=minQ, XMax=maxQ)

            mergedQ = (Cf+shift*Nf+Cr)/(Nf/scale + Nr)
            if consider_can:
                mergedQ -= (Cf_can+Cr_can)/(Nf_can/scale + Nr_can)

            RenameWorkspace(InputWorkspace=mergedQ,OutputWorkspace= retWSname_merged)

            # save the properties Transmission and TransmissionCan inside the merged workspace
            # get these values from the rear_workspace because they are the same value as the front one.
            # ticket #6929
            rear_ws = mtd[retWSname_rear]
            for prop in ['Transmission','TransmissionCan']:
                if rear_ws.getRun().hasProperty(prop):
                    ws_name = rear_ws.getRun().getLogData(prop).value
                    if mtd.doesExist(ws_name): # ensure the workspace has not been deleted
                        AddSampleLog(Workspace=retWSname_merged,LogName= prop, LogText=ws_name)
        else:
            issueWarning('rear and front data has no overlapping q-region. Merged workspace no calculated')

        delete_workspaces(retWSname_rear+"_sumOfCounts")
        delete_workspaces(retWSname_rear+"_sumOfNormFactors")
        delete_workspaces(retWSname_front+"_sumOfCounts")
        delete_workspaces(retWSname_front+"_sumOfNormFactors")
        if consider_can:
            delete_workspaces(retWSname_front+"_can_tmp_sumOfNormFactors")
            delete_workspaces(retWSname_rear+"_can_tmp_sumOfNormFactors")
            delete_workspaces(retWSname_front+"_can_tmp_sumOfCounts")
            delete_workspaces(retWSname_rear+"_can_tmp_sumOfCounts")

        retWSname = retWSname_merged

    #applying scale and shift on the front detector reduced data
    if reduce_front_flag:
        frontWS = mtd[retWSname_front]
        frontWS = (frontWS+shift)*scale
        RenameWorkspace(InputWorkspace=frontWS,OutputWorkspace= retWSname_front)

    # finished calculating cross section so can restore these value
    ReductionSingleton().to_Q.outputParts = toRestoreOutputParts
    ReductionSingleton().instrument.setDetector(toRestoreAfterAnalysis)

    # update the scale and shift values of out_fit_settings
    out_fit_settings['scale'] = ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale
    out_fit_settings['shift'] = ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift

    if resetSetup:
        _refresh_singleton()

    # Relabel the YUnit of the resulting workspaces before we return anything.
    # Depending on the given options, we may have rear, front and merged
    # workspaces to handle.  These may also be WorkspaceGroups.
    for ws_name in [retWSname_rear, retWSname_front, retWSname_merged]:
        if not ws_name in mtd:
            continue
        ws = mtd[ws_name]
        if isinstance(ws, WorkspaceGroup):
            relabel_ws_list = [mtd[name] for name in ws.getNames()]
        else:
            relabel_ws_list = [ws]
        for relabel_ws in relabel_ws_list:
            relabel_ws.setYUnitLabel("I(q) (cm-1)")

    return retWSname

def _fitRescaleAndShift(rAnds, frontData, rearData):
    """
        Fit rear data to FRONTnew(Q) = ( FRONT(Q) + SHIFT )xRESCALE,
        FRONT(Q) is the frontData argument. Returns scale and shift

        @param rAnds: A DetectorBank -> _RescaleAndShift structure
        @param frontData: Reduced front data
        @param rearData: Reduced rear data
    """
    if rAnds.fitScale==False and rAnds.fitShift==False:
        return rAnds.scale, rAnds.shift
    #TODO: we should allow the user to add constraints?
    if rAnds.fitScale==False:
        if rAnds.qRangeUserSelected:
            Fit(InputWorkspace=rearData,
                Function='name=TabulatedFunction, Workspace="'+str(frontData)+'"'
                +";name=FlatBackground", Ties='f0.Scaling='+str(rAnds.scale),
                Output="__fitRescaleAndShift", StartX=rAnds.qMin, EndX=rAnds.qMax)
        else:
            Fit(InputWorkspace=rearData,
                Function='name=TabulatedFunction, Workspace="'+str(frontData)+'"'
                +";name=FlatBackground", Ties='f0.Scaling='+str(rAnds.scale),
                Output="__fitRescaleAndShift")
    elif rAnds.fitShift==False:
        if rAnds.qRangeUserSelected:
            function_input = 'name=TabulatedFunction, Workspace="'+str(frontData)+'"' +";name=FlatBackground"
            ties = 'f1.A0='+str(rAnds.shift*rAnds.scale)
            logger.warning('function input ' + str(function_input))

            Fit(InputWorkspace=rearData,
                Function='name=TabulatedFunction, Workspace="'+str(frontData)+'"'
                +";name=FlatBackground", Ties='f1.A0='+str(rAnds.shift*rAnds.scale),
                Output="__fitRescaleAndShift", StartX=rAnds.qMin, EndX=rAnds.qMax)
        else:
            Fit(InputWorkspace=rearData,
                Function='name=TabulatedFunction, Workspace="'+str(frontData)+'"'
                +";name=FlatBackground", Ties='f1.A0='+str(rAnds.shift*rAnds.scale),
                Output="__fitRescaleAndShift")
    else:
        if rAnds.qRangeUserSelected:
            Fit(InputWorkspace=rearData,
                Function='name=TabulatedFunction, Workspace="'+str(frontData)+'"'
                +";name=FlatBackground",
                Output="__fitRescaleAndShift", StartX=rAnds.qMin, EndX=rAnds.qMax)
        else:
            Fit(InputWorkspace=rearData, Function='name=TabulatedFunction, Workspace="'+str(frontData)+'"'
                +";name=FlatBackground",Output="__fitRescaleAndShift")

    param = mtd['__fitRescaleAndShift_Parameters']

    row1 = param.row(0).items()
    row2 = param.row(1).items()
    row3 = param.row(2).items()
    scale = row1[1][1]
    chiSquared = row3[1][1]

    fitSuccess = True
    if not chiSquared > 0:
        issueWarning("Can't fit front detector RESCALE or SHIFT. Use non fitted values")
        fitSuccess = False
    if scale == 0.0:
        issueWarning("front detector RESCALE fitted to zero. Use non fitted values")
        fitSuccess = False

    if fitSuccess == False:
        return rAnds.scale, rAnds.shift

    shift = row2[1][1] / scale

    delete_workspaces('__fitRescaleAndShift_Parameters')
    delete_workspaces('__fitRescaleAndShift_NormalisedCovarianceMatrix')
    delete_workspaces('__fitRescaleAndShift_Workspace')

    return scale, shift

def _WavRangeReduction(name_suffix=None):
    """
        Run a reduction that has been set up, from loading the raw data to calculating Q
    """
    def _setUpPeriod(period):
        assert ReductionSingleton().get_sample().loader.move2ws(period)
        can = ReductionSingleton().get_can()
        if can and can.loader.periods_in_file > 1:
            can.loader.move2ws(period)

        for trans in [ReductionSingleton().samp_trans_load, ReductionSingleton().can_trans_load]:
            if trans and trans.direct.periods_in_file > 1 and trans.trans.periods_in_file > 1:
                trans.direct.move2ws(period)
                trans.trans.move2next(period)
        return

    def _applySuffix(result, name_suffix):
        if name_suffix:
            old = result
            result += name_suffix
            RenameWorkspace(InputWorkspace=old,OutputWorkspace= result)
        return result

    def _common_substring(val1, val2):
        l = []
        for i in range(len(val1)):
            if val1[i]==val2[i]: l.append(val1[i])
            else:
                return ''.join(l)

    def _group_workspaces(list_of_values, outputname):
        allnames = ','.join(list_of_values)
        GroupWorkspaces(InputWorkspaces=allnames, OutputWorkspace=outputname)

    def _reduceAllSlices():
        if ReductionSingleton().getNumSlices() > 1:
            slices = []
            for index in range(ReductionSingleton().getNumSlices()):
                ReductionSingleton().setSliceIndex(index)
                slices.append(ReductionSingleton()._reduce())
            ReductionSingleton().setSliceIndex(0)
            group_name = _common_substring(slices[0], slices[1])
            if group_name[-2] == "_":
                group_name = group_name[:-2]
            _group_workspaces(slices, group_name)
            return group_name
        else:
            return ReductionSingleton()._reduce()



    result = ""
    if ReductionSingleton().get_sample().loader.periods_in_file == 1:
        result = _reduceAllSlices()
        return _applySuffix(result, name_suffix)

    calculated = []
    try:
        for period in ReductionSingleton().get_sample().loader.entries:
            _setUpPeriod(period)
            calculated.append(_reduceAllSlices())

    finally:
        if len(calculated) > 0:
            result = ReductionSingleton().get_out_ws_name(show_period=False)
            _group_workspaces(calculated, result)

    return _applySuffix(result, name_suffix)

def delete_workspaces(workspaces):
    """
        Delete the list of workspaces if possible but fail siliently if there is
        a problem
        @param workspaces: the list to delete
    """
    if type(workspaces) != type(list()):
        if type(workspaces) != type(tuple()):
            workspaces = [workspaces]

    for wksp in workspaces:
        if wksp and wksp in mtd:
            try:
                DeleteWorkspace(Workspace=wksp)
            except:
                #we're only deleting to save memory, if the workspace really won't delete leave it
                pass

def CompWavRanges(wavelens, plot=True, combineDet=None, resetSetup=True):
    """
        Compares the momentum transfer results calculated from different wavelength ranges. Given
        the list of wave ranges [a, b, c] it reduces for wavelengths a-b, b-c and a-c.
        @param wavelens: the list of wavelength ranges
        @param plot: set this to true to plot the result (must be run in Mantid), default is true
        @param combineDet: see description in WavRangeReduction
        @param resetSetup: if true reset setup at the end
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


    calculated = [WavRangeReduction(wav_start=wavelens[0], wav_end=wavelens[len(wavelens)-1], combineDet=combineDet,resetSetup=False)]
    for i in range(0, len(wavelens)-1):
        calculated.append(WavRangeReduction(wav_start=wavelens[i], wav_end=wavelens[i+1], combineDet=combineDet, resetSetup=False))

    if resetSetup:
        _refresh_singleton()

    if plot and mantidplot:
        mantidplot.plotSpectrum(calculated, 0)

    #return just the workspace name of the full range
    return calculated[0]

def PhiRanges(phis, plot=True):
    """
        Given a list of phi ranges [a, b, c, d] it reduces in the phi ranges a-b and c-d
        @param phis: the list of phi ranges
        @param plot: set this to true to plot the result (must be run in Mantid), default is true
    """

    _printMessage('PhiRanges( %s,plot=%s)'%(str(phis),plot))

    #todo covert their string into Python array

    if len(phis)/2 != float(len(phis))/2.:
        raise RuntimeError('Phi ranges must be given as pairs')

    try:
        #run the reductions, calculated will be an array with the names of all the workspaces produced
        calculated = []
        for i in range(0, len(phis), 2):
            SetPhiLimit(phis[i],phis[i+1])
            #reducedResult = ReductionSingleton()._reduce()
            #RenameWorkspace(reducedResult,'bob')
            #calculated.append(reducedResult)
            calculated.append(ReductionSingleton()._reduce())
            ReductionSingleton.replace(ReductionSingleton().cur_settings())
    finally:
        _refresh_singleton()

    if plot and mantidplot:
        mantidplot.plotSpectrum(calculated, 0)

    #return just the workspace name of the full range
    return calculated[0]

def Reduce():
    try:
        result = ReductionSingleton()._reduce()
    finally:
        _refresh_singleton()

    return result

def _SetWavelengthRange(start, end):
    ReductionSingleton().to_wavelen.set_range(start, end)

def Set1D():
    _printMessage('Set1D()')
    ReductionSingleton().set_Q_output_type('1D')

def Set2D():
    _printMessage('Set2D()')
    ReductionSingleton().set_Q_output_type('2D')

def SetDetectorFloodFile(filename, detector_name="REAR"):
    ReductionSingleton().prep_normalize.setPixelCorrFile(filename, detector_name)

def SetPhiLimit(phimin, phimax, use_mirror=True):
    """
        Call this function to restrict the analyse segments of the detector. Phimin and
        phimax define the limits of the segment where phi=0 is the -x axis and phi = 90
        is the y-axis. Setting use_mirror to true includes a second segment to be included
        it is the same as the first but rotated 180 degrees.
        @param phimin: the minimum phi angle to include
        @param phimax: the upper limit on phi for the segment
        @param use_mirror: when True (default) another segment is included, rotated 180 degrees from the first
    """
    _printMessage("SetPhiLimit(" + str(phimin) + ', ' + str(phimax) + ',use_mirror='+str(use_mirror)+')')
    #a beam centre of [0,0,0] makes sense if the detector has been moved such that beam centre is at [0,0,0]
    ReductionSingleton().mask.set_phi_limit(phimin, phimax, use_mirror)

def SetDetectorOffsets(bank, x, y, z, rot, radius, side, xtilt=0.0, ytilt=0.0 ):
    # 10/03/15 RKH added 2 more parameters - xtilt & ytilt
    """
        Adjust detector position away from position defined in IDF. On SANS2D the detector
        banks can be moved around. This method allows fine adjustments of detector bank position
        in the same way as the DET/CORR userfile command works. Hence please see
        http://www.mantidproject.org/SANS_User_File_Commands#DET for details.

        Note, for now, this command will only have an effect on runs loaded
        after this command have been executed (because it is when runs are loaded
        that components are moved away from the positions set in the IDF)

        @param bank: Must be either 'front' or 'rear' (not case sensitive)
        @param x: shift in mm
        @param y: shift in mm
        @param z: shift in mm
        @param rot: shift in degrees
        @param radius: shift in mm
        @param side: shift in mm
        @param side: xtilt in degrees
        @param side: ytilt in degrees
    """
    _printMessage("SetDetectorOffsets(" + str(bank) + ', ' + str(x)
                  + ','+str(y) + ',' + str(z) + ',' + str(rot)
                  + ',' + str(radius) + ',' + str(side) + ',' + str(xtilt)+ ',' + str(ytilt) +')')

    detector = ReductionSingleton().instrument.getDetector(bank)
    detector.x_corr = x
    detector.y_corr = y
    detector.z_corr = z
    detector.rot_corr = rot
    detector.radius_corr = radius
    detector.side_corr = side
	# 10/03/15 RKH add 2 more
    detector.x_tilt = xtilt
    detector.y_tilt = ytilt

def SetCorrectionFile(bank, filename):
    # 10/03/15 RKH, create a new routine that allows change of "direct beam file" = correction file, for a given
    # detector, this simplify the iterative process used to adjust it. Will still have to keep changing the name of the file
    # for each iteratiom to avoid Mantid using a cached version, but can then use only a single user (=mask) file for each set of iterations.
    # Modelled this on SetDetectorOffsets above ...
    """
        @param bank: Must be either 'front' or 'rear' (not case sensitive)
        @param filename: self explanatory
    """
    _printMessage("SetCorrectionFile(" + str(bank) + ', ' + filename +')')

    detector = ReductionSingleton().instrument.getDetector(bank)
    detector.correction_file = filename

def LimitsR(rmin, rmax, quiet=False, reducer=None):
    if reducer == None:
        reducer = ReductionSingleton().reference()

    if not quiet:
        _printMessage('LimitsR(' + str(rmin) + ', ' +str(rmax) + ')', reducer)

    reducer.mask.set_radi(rmin, rmax)
    reducer.CENT_FIND_RMIN = float(rmin)/1000.
    reducer.CENT_FIND_RMAX = float(rmax)/1000.

def LimitsWav(lmin, lmax, step, bin_type):
    _printMessage('LimitsWav(' + str(lmin) + ', ' + str(lmax) + ', ' + str(step) + ', '  + bin_type + ')')

    if  bin_type.upper().strip() == 'LINEAR': bin_type = 'LIN'
    if  bin_type.upper().strip() == 'LOGARITHMIC': bin_type = 'LOG'
    if bin_type == 'LOG':
        bin_sym = '-'
    else:
        bin_sym = ''

    ReductionSingleton().to_wavelen.set_rebin(lmin, bin_sym + str(step), lmax)

def LimitsQXY(qmin, qmax, step, type):
    """
        To set the bin parameters for the algorithm Qxy()
        @param qmin: the first Q value to include
        @param qmaz: the last Q value to include
        @param step: bin width
        @param type: pass LOG for logarithmic binning
    """
    _printMessage('LimitsQXY(' + str(qmin) + ', ' + str(qmax) +', ' + str(step) + ', ' + str(type) + ')')
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError('MaskFile() first')

    settings.readLimitValues('L/QXY ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + type, ReductionSingleton())

def SetEventSlices(input_str):
    """
    """
    ReductionSingleton().setSlicesLimits(input_str)


def PlotResult(workspace, canvas=None):
    """
        Draws a graph of the passed workspace. If the workspace is 2D (has many spectra
        a contour plot is written
        @param workspace: a workspace name or handle to plot
        @param canvas: optional handle to an existing graph to write the plot to
        @return: a handle to the graph that was written to
    """
    if not mantidplot:
        issueWarning('Plot functions are not available, is this being run from outside Mantidplot?')
        return

    #ensure that we are dealing with a workspace handle rather than its name
    workspace = mtd[str(workspace)]
    if isinstance(workspace, WorkspaceGroup):
        numSpecs = workspace[0].getNumberHistograms()
    else:
        numSpecs = workspace.getNumberHistograms()

    if numSpecs == 1:
        graph = mantidplot.plotSpectrum(workspace,0)
    else:
        graph = mantidplot.importMatrixWorkspace(workspace.getName()).plotGraph2D()

    if not canvas is None:
        #we were given a handle to an existing graph, use it
        mantidplot.mergePlots(canvas, graph)
        graph = canvas

    return graph

##################### View mask details #####################################################

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
        samp = LAST_SAMPLE

        if samp:
            CloneWorkspace(InputWorkspace=samp, OutputWorkspace=mask_worksp)

            if su.isEventWorkspace(samp):
                assert samp + "_monitors" in mtd
                CloneWorkspace(InputWorkspace=samp + "_monitors",
                               OutputWorkspace=mask_worksp + "_monitors")
                su.fromEvent2Histogram(mask_worksp, mtd[mask_worksp + "_monitors"])

            counts_data = '__DisplayMasked_tempory_wksp'
            Integration(InputWorkspace=mask_worksp,OutputWorkspace= counts_data)

        else:
            instrument.load_empty(mask_worksp)
            instrument.set_up_for_run('emptyInstrument')

    ReductionSingleton().mask.display(mask_worksp, ReductionSingleton(), counts_data)
    if counts_data:
        DeleteWorkspace(counts_data)

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

def FindBeamCentre(rlow, rupp, MaxIter = 10, xstart = None, ystart = None, tolerance=1.251e-4):
    """
        Estimates the location of the effective beam centre given a good initial estimate. For more
        information go to this page
        mantidproject.org/Using_the_SANS_GUI_Beam_Centre_Finder
        @param rlow: mask around the (estimated) centre to this radius (in millimetres)
        @param rupp: don't include further out than this distance (mm) from the centre point
        @param MaxInter: don't calculate more than this number of iterations (default = 10)
        @param xstart: initial guess for the horizontal distance of the beam centre from the detector centre in meters (default the values in the mask file)
        @param ystart: initial guess for the distance of the beam centre from the detector centre vertically in metres (default the values in the mask file)
    @param tolerance: define the precision of the search. If the step is smaller than the tolerance, it will be considered stop searching the centre (default=1.251e-4 or 1.251um)
        @return: the best guess for the beam centre point
    """
    XSTEP = ReductionSingleton().inst.cen_find_step
    YSTEP = ReductionSingleton().inst.cen_find_step2

    XSF = ReductionSingleton().inst.beam_centre_scale_factor1
    YSF = ReductionSingleton().inst.beam_centre_scale_factor2

    original = ReductionSingleton().get_instrument().cur_detector_position(ReductionSingleton().get_sample().get_wksp_name())

    if ReductionSingleton().instrument.lowAngDetSet:
        det_bank = 'rear'
    else:
        det_bank = 'front'

    if xstart or ystart:
        ReductionSingleton().set_beam_finder(
            isis_reduction_steps.BaseBeamFinder(\
            float(xstart), float(ystart)),det_bank)

    beamcoords = ReductionSingleton().get_beam_center()
    XNEW = beamcoords[0]
    YNEW = beamcoords[1]
    xstart = beamcoords[0]
    ystart = beamcoords[1]


    #remove this if we know running the Reducer() doesn't change i.e. all execute() methods are const
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

    centre = CentreFinder(original)
    centre.logger.notice("xstart,ystart="+str(XNEW*1000.)+" "+str(YNEW*1000.))
    centre.logger.notice("Starting centre finding routine ...")
    #this function moves the detector to the beam center positions defined above and returns an estimate of where the beam center is relative to the new center
    resX_old, resY_old = centre.SeekCentre(centre_reduction, [XNEW, YNEW])
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

    logger.notice(centre.status_str(0, resX_old, resY_old))

    # take first trial step
    XNEW = xstart + XSTEP
    YNEW = ystart + YSTEP
    graph_handle = None
    it = 0
    for i in range(1, MaxIter+1):
        it = i

        centre_reduction.set_beam_finder(
            isis_reduction_steps.BaseBeamFinder(XNEW, YNEW), det_bank)

        resX, resY = centre.SeekCentre(centre_reduction, [XNEW, YNEW])
        centre_reduction = copy.deepcopy(ReductionSingleton().reference())
        LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

        centre.logger.notice(centre.status_str(it, resX, resY))

        if mantidplot:
            try :
                if not graph_handle:
                    #once we have a plot it will be updated automatically when the workspaces are updated
                    graph_handle = mantidplot.plotSpectrum(centre.QUADS, 0)
                graph_handle.activeLayer().setTitle(\
                        centre.status_str(it, resX, resY))
            except :
                #if plotting is not available it probably means we are running outside a GUI, in which case do everything but don't plot
                pass

        #have we stepped across the y-axis that goes through the beam center?
        if resX > resX_old:
            # yes with stepped across the middle, reverse direction and half the step size
            XSTEP = -XSTEP/2.
        if resY > resY_old:
            YSTEP = -YSTEP/2.
        if abs(XSTEP) < tolerance and abs(YSTEP) < tolerance :
            # this is the success criteria, we've close enough to the center
            centre.logger.notice("Converged - check if stuck in local minimum!")
            break

        resX_old = resX
        resY_old = resY
        XNEW += XSTEP
        YNEW += YSTEP

    if it == MaxIter:
        centre.logger.notice("Out of iterations, new coordinates may not be the best!")
        XNEW -= XSTEP
        YNEW -= YSTEP

    ReductionSingleton().set_beam_finder(
        isis_reduction_steps.BaseBeamFinder(XNEW, YNEW), det_bank)
    centre.logger.notice("Centre coordinates updated: [" + str(XNEW*XSF) + ", " + str(YNEW*YSF) + ']')

    return XNEW, YNEW

###############################################################################
######################### Start of Deprecated Code ############################
###############################################################################

@deprecated
def UserPath(path):
    """
        Sets the directory in which Mantid should look for the mask file if a
        full path was not specified
        @param path: the full path to the directory
    """
    _printMessage('UserPath("' + path + '") #Will look for mask file here')
    ReductionSingleton().user_file_path = path

@deprecated
def DataPath(path):
    """
        Sets an extra directory for Mantid to look for run files
        @param path: the full path to a directory containing the run files to analyse
    """
    ReductionSingleton().set_data_path(path)

@deprecated
def CropToDetector(inputWSname, outputWSname=None):
    """
        Crops the workspace so that it only contains the spectra that correspond
        to the detectors used in the reduction
        @param inputWSname: name of the workspace to crop
        @param outputWSname: name the workspace will take (default is the inputWSname)
    """
    if not outputWSname:
        outputWSname = inputWSname

    ReductionSingleton().instrument.cur_detector().crop_to_detector(inputWSname, outputWSname)

@deprecated
def SetRearEfficiencyFile(filename):
    rear_det = ReductionSingleton().instrument.getDetector('rear')
    rear_det.correction_file = filename

@deprecated
def SetFrontEfficiencyFile(filename):
    front_det = ReductionSingleton().instrument.getDetector('front')
    front_det.correction_file = filename

@deprecated
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

@deprecated
def displayMaskFile():
    displayUserFile()

@deprecated
def displayGeometry():
    [x, y] = ReductionSingleton().get_beam_center()
    print 'Beam centre: [' + str(x) + ',' + str(y) + ']'
    print ReductionSingleton().get_sample().geometry

@deprecated
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
        _printMessage('LimitsQ(' + str(qmin) + ', ' + str(qmax) +', ' + str(step) + ','  + str(step_type) + ')')
        settings.readLimitValues('L/Q ' + str(qmin) + ' ' + str(qmax) + ' ' + str(step) + '/'  + step_type, ReductionSingleton())
    else:
        issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")

@deprecated
def ViewCurrentMask():
    """
        In MantidPlot this opens InstrumentView to display the masked
        detectors in the bank in a different colour
    """
    ReductionSingleton().ViewCurrentMask()

###############################################################################
########################## End of Deprecated Code #############################
###############################################################################

#this is like a #define I'd like to get rid of it because it seems meaningless here
DefaultTrans = 'True'
NewTrans = 'False'

_refresh_singleton()

if __name__ == '__main__':
    SetVerboseMode(True)
    SANS2D()
    MaskFile('MASKSANS2D_123T_4m_Xpress_8mm.txt')
    Set1D()
    AssignSample('SANS2D00002500.nxs')
    Gravity(True)
    wav1 = 2.0
    wav2 = wav1 + 2.0
    reduced = WavRangeReduction(wav1, wav2, DefaultTrans)
