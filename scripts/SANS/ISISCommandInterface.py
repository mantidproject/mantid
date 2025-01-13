# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines, invalid-name, redefined-builtin, protected-access, too-many-arguments
"""
Enables the SANS commands (listed at http://www.mantidproject.org/SANS) to
be run
"""

import isis_instrument
from reducer_singleton import ReductionSingleton
import isis_reduction_steps
import isis_reducer
from centre_finder import is_workspace_which_requires_angle, BeamCenterLogger, CentreFinder, CentrePositioner, FindDirectionEnum
from mantid.api import mtd, AnalysisDataService, ExperimentInfo, WorkspaceGroup
from mantid.kernel import config, Logger
from mantid.simpleapi import AddSampleLog, CloneWorkspace, DeleteWorkspace, GroupWorkspaces, RenameWorkspace, Scale
import copy
import os
from SANSadd2 import add_runs
import SANSUtility as su
from SANSUtility import deprecated
import SANSUserFileParser as UserFileParser

import warnings

warnings.simplefilter("default", category=DeprecationWarning)
warnings.warn(
    "This ISIS Command Interface is deprecated.\n"
    "Please change 'import ISISCommandInterface' or 'from ISISCommandInterface'"
    "to use 'sans.command_interface.ISISCommandInterface' instead.",
    DeprecationWarning,
    stacklevel=2,
)
warnings.simplefilter("ignore", category=DeprecationWarning)

sanslog = Logger("SANS")

try:
    from qtpy.QtWidgets import qApp

    def appwidgets():
        return qApp.allWidgets()

except (ImportError, RuntimeError):

    def appwidgets():
        return []


_VERBOSE_ = False
LAST_SAMPLE = None


def SetVerboseMode(state):
    # TODO: this needs to be on the reducer
    # _VERBOSE_ = state # FIXME this does nothing
    pass


# Print a message and log it if the
def _printMessage(msg, log=True, no_console=False):
    if log and _VERBOSE_:
        sanslog.notice(msg)
    if not no_console:
        print(msg)


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
    _printMessage("SANS2D()")
    try:
        instrument = isis_instrument.SANS2D(idf_path)
        if instrument is None:
            raise RuntimeError("The provided idf path seems to have been incorrect")
        ReductionSingleton().set_instrument(instrument)
        config["default.instrument"] = "SANS2D"
    except (Exception, Warning):
        return False
    return True


def SANS2DTUBES():
    """
    Quick, temporary workaround for the IDF problem we're fixing in #9367.
    Simply pass the correct IDF to SANS2D().
    """
    return SANS2D("SANS2D_Definition_Tubes.xml")


def LOQ(idf_path="LOQ_Definition_20020226-.xml"):
    """
    Initialises the instrument settings for LOQ
    @return True on success
    """
    _printMessage("LOQ()")
    try:
        instrument = isis_instrument.LOQ(idf_path)
        if instrument is None:
            raise RuntimeError("The provided idf path seems to have been incorrect")
        ReductionSingleton().set_instrument(instrument)
        config["default.instrument"] = "LOQ"
    except (Exception, Warning):
        return False
    return True


def LARMOR(idf_path=None):
    """
    Initialises the instrument settings for LARMOR
    @param idf_path :: optionally specify the path to the LARMOR IDF to use.
                       Uses default if none specified.
    @return True on success
    """
    _printMessage("LARMOR()")
    try:
        instrument = isis_instrument.LARMOR(idf_path)
        if instrument is None:
            raise RuntimeError("The provided idf path seems to have been incorrect")
        ReductionSingleton().set_instrument(instrument)
        config["default.instrument"] = "LARMOR"
    except (Exception, Warning):
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
    ReductionSingleton().mask.parse_instruction(ReductionSingleton().instrument.name(), details)


def MaskFile(file_name):
    """
    Loads the settings file. The settings are loaded as soon as this line is encountered
    and are overridden by other Python commands
    @param file_name: the settings file
    """
    _printMessage('#Opening "' + file_name + '"')

    # ensure that no slice string is kept from previous executions.
    ReductionSingleton().setSlicesLimits("")

    ReductionSingleton().user_settings = isis_reduction_steps.UserFile(file_name)
    status = ReductionSingleton().user_settings.execute(ReductionSingleton(), None)
    _printMessage('#Success reading "' + file_name + '"' + " is " + str(status))
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


def Gravity(flag, extra_length=0.0):
    _printMessage("Gravity(" + str(flag) + ", " + str(extra_length) + ")")
    ReductionSingleton().to_Q.set_gravity(flag)
    ReductionSingleton().to_Q.set_extra_length(extra_length)


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
    ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift = (
        ReductionSingleton().instrument.getDetector("FRONT")._RescaleAndShift(scale, shift, fitScale, fitShift, qMin, qMax)
    )
    _printMessage("#Set front detector rescale/shift values")


def SetMergeQRange(q_min=None, q_max=None):
    """
    Stores property about the detector which is used to specify merge range.
    @param qMin: When set to None (default) then for merge use the overlapping q region of front and rear detectors
    @param qMax: When set to None (default) then for merge use the overlapping q region of front and rear detectors
    """
    ReductionSingleton().instrument.getDetector("FRONT").mergeRange = (
        ReductionSingleton().instrument.getDetector("FRONT")._MergeRange(q_min, q_max)
    )
    _printMessage("#Set merge range values")


def SetDetectorMaskFiles(filenames):
    assert isinstance(filenames, str), "Expected a command seperated list of filenames, got %r instead" % filenames
    ReductionSingleton().settings["MaskFiles"] = filenames
    _printMessage("#Set masking file names to {0}".format(filenames))


def SetMonDirect(correction_file):
    if not ReductionSingleton().instrument:
        raise RuntimeError("You must create an instrument object first")

    ReductionSingleton().instrument.cur_detector().correction_file = correction_file
    ReductionSingleton().instrument.other_detector().correction_file = correction_file
    _printMessage("#Set MonDirect to {0}".format(correction_file))


def TransFit(mode, lambdamin=None, lambdamax=None, selector="BOTH"):
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
    if lambdamin:
        message += ", " + str(lambdamin)
    if lambdamax:
        message += ", " + str(lambdamax)
    message += ", selector=" + selector
    _printMessage('TransFit("' + message + '")')

    ReductionSingleton().set_trans_fit(lambdamin, lambdamax, mode, selector)


def TransWorkspace(sample, can=None):
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
        except (Exception, Warning):
            pass
    return ws_name, logs


def AssignCan(can_run, reload=True, period=isis_reduction_steps.LoadRun.UNSET_PERIOD):
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
        mes += ", " + str(period)
    mes += ")"
    _printMessage(mes)

    ReductionSingleton().set_can(can_run, reload, period)
    return _return_old_compatibility_assign_methods(ReductionSingleton().get_can().wksp_name)


def TransmissionSample(sample, direct, reload=True, period_t=-1, period_d=-1):
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
    return ReductionSingleton().samp_trans_load.execute(ReductionSingleton(), None)


def TransmissionCan(can, direct, reload=True, period_t=-1, period_d=-1):
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
    return ReductionSingleton().can_trans_load.execute(ReductionSingleton(), None)


def AssignSample(sample_run, reload=True, period=isis_reduction_steps.LoadRun.UNSET_PERIOD):
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
        mes += ", " + str(period)
    mes += ")"
    _printMessage(mes)

    ReductionSingleton().set_sample(sample_run, reload, period)

    global LAST_SAMPLE
    LAST_SAMPLE = ReductionSingleton().get_sample().wksp_name
    return _return_old_compatibility_assign_methods(LAST_SAMPLE)


def SetCentre(xcoord, ycoord, bank="rear"):
    """
    Configure the Beam Center position. It support the configuration of the centre for
    the both detectors bank (low-angle bank and high-angle bank detectors)

    It allows defining the position for both detector banks.
    :param xcoord: X position of beam center in the user coordinate system.
    :param ycoord: Y position of beam center in the user coordinate system.
    :param bank: The selected bank ('rear' - low angle or 'front' - high angle)
    Introduced #5942
    """
    _printMessage("SetCentre(" + str(xcoord) + ", " + str(ycoord) + ")")
    # use the scale factors from the parameter file to scale correctly
    XSF = ReductionSingleton().inst.beam_centre_scale_factor1
    YSF = ReductionSingleton().inst.beam_centre_scale_factor2

    ReductionSingleton().set_beam_finder(isis_reduction_steps.BaseBeamFinder(float(xcoord) / XSF, float(ycoord) / YSF), bank)


def GetMismatchedDetList():
    """
    Return the list of mismatched detector names
    """
    return ReductionSingleton().instrument.get_marked_dets()


# pylint: disable = too-many-branches


def WavRangeReduction(  # noqa: C901
    wav_start=None,
    wav_end=None,
    full_trans_wav=None,
    name_suffix=None,
    combineDet=None,
    resetSetup=True,
    out_fit_settings=dict(),
):
    """
    Run reduction from loading the raw data to calculating Q. Its optional arguments allows specifics
    details to be adjusted, and optionally the old setup is reset at the end. Note if FIT of RESCALE or SHIFT
    is selected then both REAR and FRONT detectors are both reduced EXCEPT if only the REAR detector is selected
    to be reduced

    @param wav_start: the first wavelength to be in the output data
    @param wav_end: the last wavelength in the output data
    @param full_trans_wav: if to use a wide wavelength range, the instrument's default wavelength range,
                           for the transmission correction, false by default
    @param name_suffix: append the created output workspace with this
    @param combineDet: combineDet can be one of the following:
                       'rear'                (run one reduction for the 'rear' detector data)
                       'front'               (run one reduction for the 'front' detector data, and rescale+shift 'front' data)
                       'both'                (run both the above two reductions)
                       'merged'              (run the same reductions as 'both' and additionally create a merged data workspace)
                        None                 (run one reduction for whatever detector has been set as the current detector
                                              before running this method. If front apply rescale+shift)
    @param resetSetup: if true reset setup at the end
    @param out_fit_settings: An output parameter. It is used, specially when resetSetup is True, in order to remember the
                             'scale and fit' of the fitting algorithm.
    @return Name of one of the workspaces created
    """
    _printMessage("WavRangeReduction(" + str(wav_start) + ", " + str(wav_end) + ", " + str(full_trans_wav) + ")")
    # these flags indicate if it is necessary to reduce the front bank, the rear bank and if it is supposed to
    # merge them
    reduce_rear_flag = False
    reduce_front_flag = False
    merge_flag = False

    retWSname_rear, retWSname_front, retWSname_merged = ["", "", ""]

    # combineDet from None to 'rear' or 'front'
    if combineDet is None:
        if ReductionSingleton().instrument.cur_detector().isAlias("FRONT"):
            combineDet = "front"
        else:
            combineDet = "rear"

    if full_trans_wav is not None:
        ReductionSingleton().full_trans_wav = full_trans_wav

    ReductionSingleton().to_wavelen.set_range(wav_start, wav_end)
    rAnds = ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift
    # check if fit is required.
    fitRequired = False
    if rAnds.fitScale or rAnds.fitShift:
        fitRequired = True

    com_det_option = combineDet.lower()

    # the only special case where reduce rear is not required is
    # if the user chose to reduce front and does not require fit
    if not (com_det_option == "front" and not fitRequired):
        reduce_rear_flag = True
    if com_det_option != "rear":
        reduce_front_flag = True
    if com_det_option == "merged":
        merge_flag = True

    # The shift and scale is always on the front detector.
    if not reduce_front_flag:
        fitRequired = False

    # To backup value of singleton which are temporarily modified in this method
    toRestoreAfterAnalysis = ReductionSingleton().instrument.cur_detector().name()
    toRestoreOutputParts = ReductionSingleton().to_Q.outputParts

    # if 'merged' then when cross section is calculated output the two individual parts
    # of the cross section. These additional outputs are required to calculate
    # the merged workspace
    if merge_flag:
        ReductionSingleton().to_Q.outputParts = True

    # do reduce rear bank data
    if reduce_rear_flag:
        ReductionSingleton().instrument.setDetector("rear")
        retWSname_rear, rear_slices = _WavRangeReduction(name_suffix)
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
            if ReductionSingleton().instrument._NAME == "LOQ" and ReductionSingleton().get_beam_center(
                "rear"
            ) != ReductionSingleton().get_beam_center("front"):
                # It is necessary to reload sample, transmission and can files.
                # reload sample
                issueWarning("Trying to reload workspaces")
                ReductionSingleton().instrument.setDetector("front")
                ReductionSingleton()._sample_run.reload(ReductionSingleton())
                # reassign can
                if ReductionSingleton().get_can():
                    ReductionSingleton().get_can().reload(ReductionSingleton())
                if ReductionSingleton().samp_trans_load:
                    # refresh Transmission
                    ReductionSingleton().samp_trans_load.execute(ReductionSingleton(), None)
                if ReductionSingleton().can_trans_load:
                    ReductionSingleton().can_trans_load.execute(ReductionSingleton(), None)

        ReductionSingleton().instrument.setDetector("front")

        retWSname_front, front_slices = _WavRangeReduction(name_suffix)
        retWSname = retWSname_front

    # This section provides a the REAR -- FRONT fitting and a stitched workspace.
    # If merge_flag is selected we use SANSStitch and get the fitting for free
    # If fitRequired is selected, then we explicitly call the SANSFitScale algorithm
    if merge_flag:
        if ReductionSingleton().getNumSlices() > 1:
            slices = []
            for index in range(ReductionSingleton().getNumSlices()):
                merge_workspace = _merge_workspaces(front_slices[index], rear_slices[index], rAnds)
                slices.append(merge_workspace)
            ReductionSingleton().setSliceIndex(0)
            group_name = _common_substring(slices[0], slices[1])
            if group_name[-2] == "_":
                group_name = group_name[:-2]
            _group_workspaces(slices, group_name)
            retWSname_merged = group_name
        else:
            retWSname_merged = _merge_workspaces(retWSname_front, retWSname_rear, rAnds)

        retWSname = retWSname_merged
    elif fitRequired:
        # Get fit parameters
        scale_factor, shift_factor, fit_mode = su.extract_fit_parameters(rAnds)

        # Since only the fit is required we use only the SANSFitScale algorithm
        kwargs_fit = {
            "HABWorkspace": mtd[retWSname_front],
            "LABWorkspace": mtd[retWSname_rear],
            "Mode": fit_mode,
            "ScaleFactor": scale_factor,
            "ShiftFactor": shift_factor,
        }
        alg_fit = su.createUnmanagedAlgorithm("SANSFitShiftScale", **kwargs_fit)
        alg_fit.execute()

        # Get the fit values
        shift_from_alg = alg_fit.getProperty("OutShiftFactor").value
        scale_from_alg = alg_fit.getProperty("OutScaleFactor").value
        ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.shift = shift_from_alg
        ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.scale = scale_from_alg

    shift = ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.shift
    scale = ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.scale

    # applying scale and shift on the front detector reduced data
    if reduce_front_flag:
        Scale(InputWorkspace=retWSname_front, OutputWorkspace=retWSname_front, Operation="Add", Factor=shift)
        Scale(InputWorkspace=retWSname_front, OutputWorkspace=retWSname_front, Operation="Multiply", Factor=scale)

    # finished calculating cross section so can restore these value
    ReductionSingleton().to_Q.outputParts = toRestoreOutputParts
    ReductionSingleton().instrument.setDetector(toRestoreAfterAnalysis)

    # update the scale and shift values of out_fit_settings
    out_fit_settings["scale"] = ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.scale
    out_fit_settings["shift"] = ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.shift

    if resetSetup:
        _refresh_singleton()

    # Relabel the YUnit of the resulting workspaces before we return anything.
    # Depending on the given options, we may have rear, front and merged
    # workspaces to handle.  These may also be WorkspaceGroups.
    for ws_name in [retWSname_rear, retWSname_front, retWSname_merged]:
        if ws_name not in mtd:
            continue
        ws = mtd[ws_name]
        if isinstance(ws, WorkspaceGroup):
            relabel_ws_list = [mtd[name] for name in ws.getNames()]
        else:
            relabel_ws_list = [ws]
        for relabel_ws in relabel_ws_list:
            relabel_ws.setYUnitLabel("I(q) (cm-1)")

    return retWSname


def _merge_workspaces(retWSname_front, retWSname_rear, rAnds):
    # Prepare the Norm and Count workspaces for the FRONT and the REAR detectors
    retWSname_merged = retWSname_rear
    if retWSname_merged.count("rear") == 1:
        retWSname_merged = retWSname_merged.replace("rear", "merged")
    elif retWSname_merged.count("main") == 1:
        retWSname_merged = retWSname_merged.replace("main", "merged")
    else:
        retWSname_merged = retWSname_merged + "_merged"

    Nf = mtd[retWSname_front + "_sumOfNormFactors"]
    Nr = mtd[retWSname_rear + "_sumOfNormFactors"]
    Cf = mtd[retWSname_front + "_sumOfCounts"]
    Cr = mtd[retWSname_rear + "_sumOfCounts"]
    consider_can = True
    try:
        Nf_can = mtd[retWSname_front + "_can_tmp_sumOfNormFactors"]
        Nr_can = mtd[retWSname_rear + "_can_tmp_sumOfNormFactors"]
        Cf_can = mtd[retWSname_front + "_can_tmp_sumOfCounts"]
        Cr_can = mtd[retWSname_rear + "_can_tmp_sumOfCounts"]
        if Cr_can is None:
            consider_can = False
    except KeyError:
        # The CAN was not specified
        consider_can = False

    # Get fit parameters
    scale_factor, shift_factor, fit_mode, fit_min, fit_max = su.extract_fit_parameters(rAnds)
    merge_range = ReductionSingleton().instrument.getDetector("FRONT").mergeRange

    kwargs_stitch = {
        "HABCountsSample": Cf,
        "HABNormSample": Nf,
        "LABCountsSample": Cr,
        "LABNormSample": Nr,
        "ProcessCan": False,
        "Mode": fit_mode,
        "ScaleFactor": scale_factor,
        "ShiftFactor": shift_factor,
        "OutputWorkspace": retWSname_merged,
        "MergeMask": merge_range.q_merge_range,
    }
    if consider_can:
        kwargs_can = {"HABCountsCan": Cf_can, "HABNormCan": Nf_can, "LABCountsCan": Cr_can, "LABNormCan": Nr_can, "ProcessCan": True}
        kwargs_stitch.update(kwargs_can)

    if rAnds.qRangeUserSelected:
        q_range_stitch = {"FitMin": fit_min, "FitMax": fit_max}
        kwargs_stitch.update(q_range_stitch)

    if merge_range.q_merge_range:
        if merge_range.q_min:
            q_range_options = {"MergeMin": merge_range.q_min}
            kwargs_stitch.update(q_range_options)
        if merge_range.q_max:
            q_range_options = {"MergeMax": merge_range.q_max}
            kwargs_stitch.update(q_range_options)

    alg_stitch = su.createUnmanagedAlgorithm("SANSStitch", **kwargs_stitch)
    alg_stitch.execute()

    # Get the fit values
    shift_from_alg = alg_stitch.getProperty("OutShiftFactor").value
    scale_from_alg = alg_stitch.getProperty("OutScaleFactor").value
    ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.shift = shift_from_alg
    ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.scale = scale_from_alg

    # Get the merged workspace
    mergedQ = alg_stitch.getProperty("OutputWorkspace").value
    # Add the output to the Analysis Data Service
    AnalysisDataService.addOrReplace(retWSname_merged, mergedQ)

    # save the properties Transmission and TransmissionCan inside the merged workspace
    # get these values from the rear_workspace because they are the same value as the front one.
    # ticket #6929
    rear_ws = mtd[retWSname_rear]
    for prop in ["Transmission", "TransmissionCan"]:
        if rear_ws.getRun().hasProperty(prop):
            ws_name = rear_ws.getRun().getLogData(prop).value
            if mtd.doesExist(ws_name):  # ensure the workspace has not been deleted
                AddSampleLog(Workspace=retWSname_merged, LogName=prop, LogText=ws_name)

    retWSname = retWSname_merged

    # Remove the partial workspaces, this needs to be done for when we merge and/or fit
    delete_workspaces(retWSname_rear + "_sumOfCounts")
    delete_workspaces(retWSname_rear + "_sumOfNormFactors")
    delete_workspaces(retWSname_front + "_sumOfCounts")
    delete_workspaces(retWSname_front + "_sumOfNormFactors")
    if consider_can:
        delete_workspaces(retWSname_front + "_can_tmp_sumOfNormFactors")
        delete_workspaces(retWSname_rear + "_can_tmp_sumOfNormFactors")
        delete_workspaces(retWSname_front + "_can_tmp_sumOfCounts")
        delete_workspaces(retWSname_rear + "_can_tmp_sumOfCounts")

    return retWSname


def _common_substring(val1, val2):
    substrings = []
    for i in range(len(val1)):
        if val1[i] == val2[i]:
            substrings.append(val1[i])
        else:
            return "".join(substrings)


def _group_workspaces(list_of_values, outputname):
    allnames = ",".join(list_of_values)
    GroupWorkspaces(InputWorkspaces=allnames, OutputWorkspace=outputname)


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
                trans.trans.move2ws(period)
        return

    def _applySuffix(result, name_suffix):
        if name_suffix:
            old = result
            result += name_suffix
            RenameWorkspace(InputWorkspace=old, OutputWorkspace=result)
        return result

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
            return group_name, slices
        else:
            return ReductionSingleton()._reduce(), None

    result = ""
    slices = []
    if ReductionSingleton().get_sample().loader.periods_in_file == 1:
        result, slices = _reduceAllSlices()
        return _applySuffix(result, name_suffix), slices

    calculated = []
    try:
        for period in ReductionSingleton().get_sample().loader.entries:
            _setUpPeriod(period)
            result, slices = _reduceAllSlices()
            calculated.append(result)

    finally:
        if len(calculated) > 0:
            result = ReductionSingleton().get_out_ws_name(show_period=False)
            _group_workspaces(calculated, result)

    return _applySuffix(result, name_suffix), slices


def delete_workspaces(workspaces):
    """
    Delete the list of workspaces if possible but fail siliently if there is
    a problem
    @param workspaces: the list to delete
    """
    if not isinstance(workspaces, type(list())):
        if not isinstance(workspaces, type(tuple())):
            workspaces = [workspaces]

    for wksp in workspaces:
        if wksp and wksp in mtd:
            try:
                DeleteWorkspace(Workspace=wksp)
            except (Exception, Warning):
                # we're only deleting to save memory, if the workspace really won't delete leave it
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

    _printMessage("CompWavRanges( %s,plot=%s)" % (str(wavelens), plot))

    if not isinstance(wavelens, type([])) or len(wavelens) < 2:
        if not isinstance(wavelens, type((1,))):
            raise RuntimeError("Error CompWavRanges() requires a list of wavelengths between which reductions will be performed.")

    calculated = [WavRangeReduction(wav_start=wavelens[0], wav_end=wavelens[len(wavelens) - 1], combineDet=combineDet, resetSetup=False)]
    for i in range(0, len(wavelens) - 1):
        calculated.append(WavRangeReduction(wav_start=wavelens[i], wav_end=wavelens[i + 1], combineDet=combineDet, resetSetup=False))

    if resetSetup:
        _refresh_singleton()

    if plot:
        raise NotImplementedError(
            "Plotting on the deprecated ISISCommandInterface required MantidPlot and is no"
            " longer implemented. Please switch to sans.command_interface.ISISCommandInterface"
        )

    # return just the workspace name of the full range
    return calculated[0]


def PhiRanges(phis, plot=True):
    """
    Given a list of phi ranges [a, b, c, d] it reduces in the phi ranges a-b and c-d
    @param phis: the list of phi ranges
    @param plot: set this to true to plot the result (must be run in Mantid), default is true
    """

    _printMessage("PhiRanges( %s,plot=%s)" % (str(phis), plot))

    # todo covert their string into Python array

    if len(phis) / 2 != float(len(phis)) / 2.0:
        raise RuntimeError("Phi ranges must be given as pairs")

    try:
        # run the reductions, calculated will be an array with the names of all the workspaces produced
        calculated = []
        for i in range(0, len(phis), 2):
            SetPhiLimit(phis[i], phis[i + 1])
            # reducedResult = ReductionSingleton()._reduce()
            # RenameWorkspace(reducedResult,'bob')
            # calculated.append(reducedResult)
            calculated.append(ReductionSingleton()._reduce())
            ReductionSingleton.replace(ReductionSingleton().cur_settings())
    finally:
        _refresh_singleton()

    if plot:
        raise NotImplementedError(
            "Plotting on the deprecated ISISCommandInterface required MantidPlot and is no"
            " longer implemented. Please switch to sans.command_interface.ISISCommandInterface"
        )

    # return just the workspace name of the full range
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
    _printMessage("Set1D()")
    ReductionSingleton().set_Q_output_type("1D")


def Set2D():
    _printMessage("Set2D()")
    ReductionSingleton().set_Q_output_type("2D")


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
    _printMessage("SetPhiLimit(" + str(phimin) + ", " + str(phimax) + ",use_mirror=" + str(use_mirror) + ")")
    # a beam centre of [0,0,0] makes sense if the detector has been moved such that beam centre is at [0,0,0]
    ReductionSingleton().mask.set_phi_limit(phimin, phimax, use_mirror)


# pylint: disable = too-many-arguments
def SetDetectorOffsets(bank, x, y, z, rot, radius, side, xtilt=0.0, ytilt=0.0):
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
    _printMessage(
        "SetDetectorOffsets("
        + str(bank)
        + ", "
        + str(x)
        + ","
        + str(y)
        + ","
        + str(z)
        + ","
        + str(rot)
        + ","
        + str(radius)
        + ","
        + str(side)
        + ","
        + str(xtilt)
        + ","
        + str(ytilt)
        + ")"
    )

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
    # 10/03/15 RKH, create a new routine that allows change of "direct beam file" = correction file,
    # for a given detector, this simplify the iterative process used to adjust it.
    # Will still have to keep changing the name of the file
    # for each iteratiom to avoid Mantid using a cached version, but can then use
    # only a single user (=mask) file for each set of iterations.
    # Modelled this on SetDetectorOffsets above ...
    """
    @param bank: Must be either 'front' or 'rear' (not case sensitive)
    @param filename: self explanatory
    """
    _printMessage("SetCorrectionFile(" + str(bank) + ", " + filename + ")")

    detector = ReductionSingleton().instrument.getDetector(bank)
    detector.correction_file = filename


def LimitsR(rmin, rmax, quiet=False, reducer=None):
    if reducer is None:
        reducer = ReductionSingleton().reference()

    if not quiet:
        _printMessage("LimitsR(" + str(rmin) + ", " + str(rmax) + ")", reducer)

    reducer.mask.set_radi(rmin, rmax)
    reducer.CENT_FIND_RMIN = float(rmin) / 1000.0
    reducer.CENT_FIND_RMAX = float(rmax) / 1000.0


def LimitsWav(lmin, lmax, step, bin_type):
    _printMessage("LimitsWav(" + str(lmin) + ", " + str(lmax) + ", " + str(step) + ", " + bin_type + ")")

    if bin_type.upper().strip() == "LINEAR":
        bin_type = "LIN"
    if bin_type.upper().strip() == "LOGARITHMIC":
        bin_type = "LOG"

    if bin_type == "LOG":
        bin_sym = "-"
    else:
        bin_sym = ""

    ReductionSingleton().to_wavelen.set_rebin(lmin, bin_sym + str(step), lmax)


def LimitsQXY(qmin, qmax, step):
    """
    To set the bin parameters for the algorithm Qxy()
    @param qmin: the first Q value to include
    @param qmaz: the last Q value to include
    @param step: bin width
    """
    _printMessage("LimitsQXY(" + str(qmin) + ", " + str(qmax) + ", " + str(step) + ")")
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError("MaskFile() first")

    settings.readLimitValues("L/QXY " + str(qmin) + " " + str(qmax) + " " + str(step), ReductionSingleton())


def SetEventSlices(input_str):
    """ """
    ReductionSingleton().setSlicesLimits(input_str)


def PlotResult(workspace, canvas=None):
    """
    Draws a graph of the passed workspace. If the workspace is 2D (has many spectra
    a contour plot is written
    @param workspace: a workspace name or handle to plot
    @param canvas: optional handle to an existing graph to write the plot to
    @return: a handle to the graph that was written to
    """
    raise NotImplementedError(
        "Plotting on the deprecated ISISCommandInterface required MantidPlot and is no"
        " longer implemented. Please switch to sans.command_interface.ISISCommandInterface"
    )


# #################### View mask details #####################################################


def DisplayMask(mask_worksp=None):
    """
    Displays masking by applying it to a workspace and displaying
    it in instrument view. If no workspace is passed a copy of the
    sample workspace is used, unless no sample was loaded and then
    an empty instrument will be shown
    @param mask_worksp: optional this named workspace will be modified and should be from the currently selected instrument
    @return the name of the workspace that was displayed
    """
    if not mask_worksp:
        mask_worksp = "__CurrentMask"
        samp = LAST_SAMPLE

        if samp:
            CloneWorkspace(InputWorkspace=samp, OutputWorkspace=mask_worksp)

            if su.isEventWorkspace(samp):
                assert samp + "_monitors" in mtd
                CloneWorkspace(InputWorkspace=samp + "_monitors", OutputWorkspace=mask_worksp + "_monitors")
                su.fromEvent2Histogram(mask_worksp, mtd[mask_worksp + "_monitors"])
        else:
            msg = "Cannot display the mask without a sample workspace"
            _printMessage(msg, log=True, no_console=False)
            return

    ReductionSingleton().mask.display(mask_worksp, ReductionSingleton())
    return mask_worksp


# Print a test script for Colette if asked
def createColetteScript(inputdata, format, reduced, centreit, plotresults, csvfile="", savepath=""):
    script = ""
    if csvfile != "":
        script += "[COLETTE]  @ " + csvfile + "\n"
    file_1 = inputdata["sample_sans"] + format
    script += "[COLETTE]  ASSIGN/SAMPLE " + file_1 + "\n"
    file_1 = inputdata["sample_trans"] + format
    file_2 = inputdata["sample_direct_beam"] + format
    if file_1 != format and file_2 != format:
        script += "[COLETTE]  TRANSMISSION/SAMPLE/MEASURED " + file_1 + " " + file_2 + "\n"
    file_1 = inputdata["can_sans"] + format
    if file_1 != format:
        script += "[COLETTE]  ASSIGN/CAN " + file_1 + "\n"
    file_1 = inputdata["can_trans"] + format
    file_2 = inputdata["can_direct_beam"] + format
    if file_1 != format and file_2 != format:
        script += "[COLETTE]  TRANSMISSION/CAN/MEASURED " + file_1 + " " + file_2 + "\n"
    if centreit:
        script += "[COLETTE]  FIT/MIDDLE"
    # Parameters
    script += "[COLETTE]  LIMIT/RADIUS " + str(ReductionSingleton().mask.min_radius)
    script += " " + str(ReductionSingleton().mask.max_radius) + "\n"
    script += "[COLETTE]  LIMIT/WAVELENGTH " + ReductionSingleton().to_wavelen.get_range() + "\n"
    if ReductionSingleton().DWAV < 0:
        script += "[COLETTE]  STEP/WAVELENGTH/LOGARITHMIC " + str(ReductionSingleton().to_wavelen.w_step)[1:] + "\n"
    else:
        script += "[COLETTE]  STEP/WAVELENGTH/LINEAR " + str(ReductionSingleton().to_wavelen.w_step) + "\n"
    # For the moment treat the rebin string as min/max/step
    qbins = ReductionSingleton().Q_REBEIN.split(",")
    nbins = len(qbins)
    if ReductionSingleton().to_Q.output_type == "1D":
        script += "[COLETTE]  LIMIT/Q " + str(qbins[0]) + " " + str(qbins[nbins - 1]) + "\n"
        dq = float(qbins[1])
        if dq < 0:
            script += "[COLETTE]  STEP/Q/LOGARITHMIC " + str(dq)[1:] + "\n"
        else:
            script += "[COLETTE]  STEP/Q/LINEAR " + str(dq) + "\n"
    else:
        script += "[COLETTE]  LIMIT/QXY " + str(0.0) + " " + str(ReductionSingleton().QXY2) + "\n"
        if ReductionSingleton().DQXY < 0:
            script += "[COLETTE]  STEP/QXY/LOGARITHMIC " + str(ReductionSingleton().DQXY)[1:] + "\n"
        else:
            script += "[COLETTE]  STEP/QXY/LINEAR " + str(ReductionSingleton().DQXY) + "\n"

    # Correct
    script += "[COLETTE] CORRECT\n"
    if plotresults:
        script += "[COLETTE]  DISPLAY/HISTOGRAM " + reduced + "\n"
    if savepath != "":
        script += "[COLETTE]  WRITE/LOQ " + reduced + " " + savepath + "\n"

    return script


def FindBeamCentre(rlow, rupp, MaxIter=10, xstart=None, ystart=None, tolerance=1.251e-4, find_direction=FindDirectionEnum.ALL):
    """
    Estimates the location of the effective beam centre given a good initial estimate. For more
    information go to this page
    mantidproject.org/Using_the_SANS_GUI_Beam_Centre_Finder
    @param rlow: mask around the (estimated) centre to this radius (in millimetres)
    @param rupp: don't include further out than this distance (mm) from the centre point
    @param MaxInter: don't calculate more than this number of iterations (default = 10)
    @param xstart: initial guess for the horizontal distance of the beam centre
                   from the detector centre in meters (default the values in the mask file), or in the
                   case of rotated instruments a rotation about the y axis. The unit is degree/XSF
    @param ystart: initial guess for the distance of the beam centre from the detector centre
                   vertically in metres (default the values in the mask file)
    @param tolerance: define the precision of the search. If the step is smaller than the
                      tolerance, it will be considered stop searching the centre (default=1.251e-4 or 1.251um)
    @param find_only: if only Up/Down or only Left/Right is
                      required then variable is set to
    @return: the best guess for the beam centre point
    """
    COORD1STEP = ReductionSingleton().inst.cen_find_step
    COORD2STEP = ReductionSingleton().inst.cen_find_step2
    XSF = ReductionSingleton().inst.beam_centre_scale_factor1
    YSF = ReductionSingleton().inst.beam_centre_scale_factor2
    coord1_scale_factor = XSF
    coord2_scale_factor = YSF

    # Here we have to be careful as the original position can be either in [m, m] or [degree, m], we need to make sure
    # that we are consistent to not mix with [degree/XSF, m]
    original = ReductionSingleton().get_instrument().cur_detector_position(ReductionSingleton().get_sample().get_wksp_name())

    if ReductionSingleton().instrument.lowAngDetSet:
        det_bank = "rear"
    else:
        det_bank = "front"

    if xstart or ystart:
        ReductionSingleton().set_beam_finder(isis_reduction_steps.BaseBeamFinder(float(xstart), float(ystart)), det_bank)

    beamcoords = ReductionSingleton().get_beam_center()

    # remove this if we know running the Reducer() doesn't change i.e. all execute() methods are const
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

    # Create an object which handles the positions and increments
    centre_positioner = CentrePositioner(
        reducer=centre_reduction,
        position_type=find_direction,
        coord1_start=beamcoords[0],
        coord2_start=beamcoords[1],
        coord1_step=COORD1STEP,
        coord2_step=COORD2STEP,
        tolerance=tolerance,
    )

    # Produce the initial position
    COORD1NEW, COORD2NEW = centre_positioner.produce_initial_position()

    # Set the CentreFinder
    sign_policy = centre_positioner.produce_sign_policy()
    centre = CentreFinder(original, sign_policy, find_direction)

    # Produce a logger for this the Beam Centre Finder
    beam_center_logger = BeamCenterLogger(centre_reduction, coord1_scale_factor, coord2_scale_factor)

    # this function moves the detector to the beam center positions defined above and
    # returns an estimate of where the beam center is relative to the new center
    resCoord1_old, resCoord2_old = centre.SeekCentre(centre_reduction, [COORD1NEW, COORD2NEW])
    centre_reduction = copy.deepcopy(ReductionSingleton().reference())
    LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)
    beam_center_logger.report_status(0, COORD1NEW, COORD2NEW, resCoord1_old, resCoord2_old)

    # If we have 0 iterations then we should return here. At this point the
    # Left/Right/Up/Down workspaces have been already created by the SeekCentre function.
    if MaxIter <= 0:
        zero_iterations_msg = (
            "You have selected 0 iterations. The beam centre "
            + "will be positioned at ("
            + str(xstart * coord1_scale_factor)
            + ", "
            + str(ystart * coord2_scale_factor)
            + ")"
        )
        beam_center_logger.report(zero_iterations_msg)
        return xstart, ystart

    beam_center_logger.report_init(COORD1NEW, COORD2NEW)

    # take first trial step
    COORD1NEW, COORD2NEW = centre_positioner.increment_position(COORD1NEW, COORD2NEW)
    for i in range(1, MaxIter + 1):
        it = i
        centre_reduction.set_beam_finder(isis_reduction_steps.BaseBeamFinder(COORD1NEW, COORD2NEW), det_bank)
        resCoord1, resCoord2 = centre.SeekCentre(centre_reduction, [COORD1NEW, COORD2NEW])

        centre_reduction = copy.deepcopy(ReductionSingleton().reference())
        LimitsR(str(float(rlow)), str(float(rupp)), quiet=True, reducer=centre_reduction)

        beam_center_logger.report_status(it, COORD1NEW, COORD2NEW, resCoord1, resCoord2)

        # have we stepped across the y-axis that goes through the beam center?
        if resCoord1 > resCoord1_old:
            # yes with stepped across the middle, reverse direction and half the step size
            centre_positioner.set_new_increment_coord1()
        if resCoord2 > resCoord2_old:
            centre_positioner.set_new_increment_coord2()
        if (
            centre_positioner.is_increment_coord1_smaller_than_tolerance()
            and centre_positioner.is_increment_coord2_smaller_than_tolerance()
        ):
            # this is the success criteria, we've close enough to the center
            beam_center_logger.report("Converged - check if stuck in local minimum!")
            break

        resCoord1_old = resCoord1
        resCoord2_old = resCoord2

        if it != MaxIter:
            COORD1NEW, COORD2NEW = centre_positioner.increment_position(COORD1NEW, COORD2NEW)
        else:
            beam_center_logger.report("Out of iterations, new coordinates may not be the best!")

    # Create the appropriate return values
    coord1_centre, coord2_centre = centre_positioner.produce_final_position(COORD1NEW, COORD2NEW)

    ReductionSingleton().set_beam_finder(isis_reduction_steps.BaseBeamFinder(coord1_centre, coord2_centre), det_bank)
    beam_center_logger.report_final(coord1_centre, coord2_centre)

    return coord1_centre, coord2_centre


###################### Utility functions ####################################################
def CreateZeroErrorFreeClonedWorkspace(input_workspace_name, output_workspace_name):
    """
    Creates a zero-error-free workspace
    @param input_workspace_name :  name of the workspace which might contain zero-error values
    @param output_workspace_name : name of the workspace which will have no zero-error values
    @return: success message
    """
    message, complete = su.create_zero_error_free_workspace(
        input_workspace_name=input_workspace_name, output_workspace_name=output_workspace_name
    )
    if not complete:
        return message
    else:
        return ""


def DeleteZeroErrorFreeClonedWorkspace(input_workspace_name):
    """
    Deletes a zero-error-free workspace
    @param input_workspace_name :  name of the workspace which might contain zero-error values
    @return: success message
    """
    message, complete = su.delete_zero_error_free_workspace(input_workspace_name=input_workspace_name)
    if not complete:
        return message
    else:
        return ""


def IsValidWsForRemovingZeroErrors(input_workspace_name):
    """
    We need to check that the workspace has been reduced, ie that it has had the Q1D or Qxy algorithm
    applied to it.
    @param input_workspace_name :  name of the input workspace
    @return: success message
    """
    message, valid = su.is_valid_ws_for_removing_zero_errors(input_workspace_name=input_workspace_name)
    if not valid:
        return message
    else:
        return ""


def check_if_event_workspace(file_name):
    """
    Checks if a file is associated with an event workspace. It tests if
    the workspace can be loaded.
    @param file_name: The file name to test
    @returns true if the workspace is an event workspace otherwise false
    """
    result = su.can_load_as_event_workspace(filename=file_name)
    print(result)
    return result


################################################################################
# Input check functions


# Check the input for time shifts when adding event files
def check_time_shifts_for_added_event_files(number_of_files, time_shifts=""):
    # If there are no entries then proceed.
    if not time_shifts or time_shifts.isspace():
        return

    time_shift_container = time_shifts.split(",")
    message = ""

    # Check if the time shift elements can be cast to float
    for time_shift_element in time_shift_container:
        try:
            float(time_shift_element)
        except ValueError:
            message = "Error: Elements of the time shift list cannot be " + "converted to a numeric value, e.g " + time_shift_element
            print(message)
            return message

    if number_of_files - 1 != len(time_shift_container):
        message = (
            "Error: Expected N-1 time shifts for N files, but read "
            + str(len(time_shift_container))
            + " time shifts for "
            + str(number_of_files)
            + " files."
        )
        print(message)
        return message


def ConvertToPythonStringList(to_convert):
    """
    Converts a python string list to a format more suitable for GUI representation
    @param to_convert:: The string list
    """
    return su.convert_to_string_list(to_convert)


def ConvertFromPythonStringList(to_convert):
    """
    Converts a comma-separated string into a Python string list
    @param to_convert:: The comm-separated string
    """
    return su.convert_from_string_list(to_convert)


###################### Accessor functions for Transmission
def GetTransmissionMonitorSpectrum():
    """
    Gets the transmission monitor spectrum. In the case of 4 or 17788 (for LOQ)
    the result is 4.
    @return: transmission monitor spectrum
    """
    transmission_monitor = ReductionSingleton().transmission_calculator.trans_mon
    if ReductionSingleton().instrument._NAME == "LOQ" and transmission_monitor == 17788:
        transmission_monitor = 4
    return transmission_monitor


def SetTransmissionMonitorSpectrum(trans_mon):
    """
    Sets the transmission monitor spectrum.
    @param trans_mon :: The spectrum to set.
    """
    if su.is_convertible_to_int(trans_mon):
        transmission_monitor = int(trans_mon)
        if transmission_monitor == 4:
            transmission_monitor = ReductionSingleton().instrument.get_m4_monitor_det_ID()
        ReductionSingleton().transmission_calculator.trans_mon = transmission_monitor
    else:
        sanslog.warning("Warning: Could not convert the transmission monitor spectrum to int.")


def UnsetTransmissionMonitorSpectrum():
    """
    Sets the transmission monitor spectrum to None
    """
    ReductionSingleton().transmission_calculator.trans_mon = None


def GetTransmissionMonitorSpectrumShift():
    """
    Gets the addditional shift for the transmission monitor spectrum.
    This currently only exists for SANS2D
    @return: transmission monitor spectrum
    """
    inst = ReductionSingleton().get_instrument()
    if inst.name() != "SANS2D" and inst.name() != "SANS2DTUBES":
        return
    return inst.monitor_4_offset


def SetTransmissionMonitorSpectrumShift(trans_mon_shift):
    """
    Sets the transmission monitor spectrum shfit.
    @param trans_mon_shift :: The spectrum shift to set.
    """
    if su.is_convertible_to_float(trans_mon_shift):
        inst = ReductionSingleton().get_instrument()
        # Note that we are only setting the transmission monitor spectrum shift
        # if we are dealing with a SANS2D instrument
        if inst.name() != "SANS2D" and inst.name() != "SANS2DTUBES":
            return
        inst.monitor_4_offset = float(trans_mon_shift)
    else:
        sanslog.warning("Warning: Could not convert transmission monitor spectrum shift to float.")


def GetTransmissionRadiusInMM():
    """
    Gets the radius for usage with beam stop as transmission monitor in mm
    @return: transmission radius in mm
    """
    radius = ReductionSingleton().transmission_calculator.radius
    if radius is not None:
        radius = radius * 1000.0
    return radius


def SetTransmissionRadiusInMM(trans_radius):
    """
    Sets the transmission monitor spectrum.
    @param trans_radius :: The radius to set in mm
    """
    if su.is_convertible_to_float(trans_radius):
        ReductionSingleton().transmission_calculator.radius = float(trans_radius) / 1000.0
    else:
        sanslog.warning("Warning: Could convert transmission radius to float.")


def GetTransmissionROI():
    """
    Gets the list of ROI file names
    @return: list of roi file names or None
    """
    roi_files = ReductionSingleton().transmission_calculator.roi_files
    if len(roi_files) == 0:
        return
    else:
        return roi_files


def SetTransmissionROI(trans_roi_files):
    """
    Sets the transmission monitor region of interest.
    @param trans_roi_files :: A string list of roi files
    """
    if su.is_valid_xml_file_list(trans_roi_files):
        ReductionSingleton().transmission_calculator.roi_files = trans_roi_files
    else:
        sanslog.warning("Warning: The roi file list does not seem to be valid.")


def GetTransmissionMask():
    """
    Gets the list of transmission mask file names
    @return: list of transmission mask file names or None
    """
    trans_mask_files = ReductionSingleton().transmission_calculator.mask_files
    if len(trans_mask_files) == 0:
        return
    else:
        return trans_mask_files


def SetTransmissionMask(trans_mask_files):
    """
    Sets the transmission masks.
    @param trans_mask_files :: A string list of mask files
    """
    if su.is_valid_xml_file_list(trans_mask_files):
        ReductionSingleton().transmission_calculator.mask_files = trans_mask_files
    else:
        sanslog.warning("Warning: The mask file list does not seem to be valid.")


def AddRuns(
    runs,
    instrument="sans2d",
    saveAsEvent=False,
    binning="Monitors",
    isOverlay=False,
    time_shifts=None,
    defType=".nxs",
    rawTypes=(".raw", ".s*", "add", ".RAW"),
    lowMem=False,
):
    """
    Method to expose the add_runs functionality for custom scripting.
    @param runs: a list with the requested run numbers
    @param instrument: the name of the selected instrument
    @param saveAsEvent: when adding event-type data, then this can be stored as event-type data
    @param binning: where to get the binnings from. This is relevant when adding Event-type data.
                    The property can be set to "Monitors" in order to emulate the binning of the monitors or to a
                    string list with the same format that is used for the Rebin algorithm. This property is ignored
                    when saving as event data.
    @param isOverlay: sets if the overlay mechanism should be used when the saveAsEvent flag is set
    @param time_shifts: provides additional time shifts if the isOverlay flag is specified. The time shifts are specified
                        in a string list. Either time_shifts is not used or a list with times in secomds. Note that there
                        has to be one entry fewer than the number of workspaces to add.
    @param defType: the file type
    @param rawTypes: the raw types
    @param lowMem: if the lowMem option should be used
    @returns a success message
    """
    # Need at least two runs to work
    if len(runs) < 1:
        issueWarning("AddRuns issue: A list with at least two runs needs to be provided.")
        return

    if time_shifts is None:
        time_shifts = []

    return add_runs(
        runs=runs,
        inst=instrument,
        defType=defType,
        rawTypes=rawTypes,
        lowMem=lowMem,
        binning=binning,
        saveAsEvent=saveAsEvent,
        isOverlay=isOverlay,
        time_shifts=time_shifts,
    )


##################### Accesor functions for QResolution
def get_q_resolution_moderator():
    """
    Gets the moderator file path
    @returns the moderator file path or nothing
    """
    val = ReductionSingleton().to_Q.get_q_resolution_moderator()
    if val is None:
        val = ""
    print(str(val))
    return val


def set_q_resolution_moderator(file_name):
    """
    Sets the moderator file path
    @param file_name: the full file path
    """
    try:
        ReductionSingleton().to_Q.set_q_resolution_moderator(file_name)
    except RuntimeError as details:
        sanslog.error(
            "The specified moderator file could not be found. Please specify a file"
            "which exists in the search directories. See details: %s" % str(details)
        )


# -- Use q resolution
def get_q_resultution_use():
    """
    Gets if the q resolution option is being used
    @returns true if the resolution option is being used, else false
    """
    val = ReductionSingleton().to_Q.get_use_q_resolution()
    print(str(val))
    return val


def set_q_resolution_use(use):
    """
    Sets if the q resolution option is being used
    @param use: use flag
    """
    if use:
        ReductionSingleton().to_Q.set_use_q_resolution(True)
    elif not use:
        ReductionSingleton().to_Q.set_use_q_resolution(False)
    else:
        sanslog.warning("Warning: Could could not set usage of QResolution")


# -- Collimation length
def get_q_resolution_collimation_length():
    """
    Get the collimation length
    @returns the collimation length in mm
    """
    element = ReductionSingleton().to_Q.get_q_resolution_collimation_length()
    msg = "CollimationLength"
    if su.is_convertible_to_float(element) or not element:
        pass
    else:
        sanslog.warning("Warning: Could not convert %s to float." % msg)
    print(str(element))
    return element


def set_q_resolution_collimation_length(collimation_length):
    """
    Sets the collimation length
    @param collimation_length: the collimation length
    """
    if collimation_length is None:
        return
    msg = "Collimation Length"
    if su.is_convertible_to_float(collimation_length):
        c_l = float(collimation_length)
        ReductionSingleton().to_Q.set_q_resolution_collimation_length(c_l)
    else:
        sanslog.warning("Warning: Could not convert %s to float." % msg)


# -- Delta R
def get_q_resolution_delta_r():
    """
    Get the delta r value
    @returns the delta r in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_delta_r, "DeltaR")
    print(str(val))
    return val


def set_q_resolution_delta_r(delta_r):
    """
    Sets the delta r value
    @param delta_r: the delta r value
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_delta_r, delta_r, "DeltaR")


# -- A1
def get_q_resolution_a1():
    """
    Get the A1 diameter
    @returns the diameter for the first aperature in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_a1, "A1")
    print(str(val))
    return val


def set_q_resolution_a1(a1):
    """
    Sets the a1 value
    @param a1: the a1 value in mm
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_a1, a1, "A1")


# -- A2
def get_q_resolution_a2():
    """
    Get the A2 diameter
    @returns the diameter for the second aperature in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_a2, "A2")
    print(str(val))
    return val


def set_q_resolution_a2(a2):
    """
    Sets the a2 value
    @param a2: the a2 value in mm
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_a2, a2, "A2")


# -- H1
def get_q_resolution_h1():
    """
    Get the first height for rectangular apertures
    @returns the first height in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_h1, "H1")
    print(str(val))
    return val


def set_q_resolution_h1(h1):
    """
    Set the first height for rectangular apertures
    @param h1: the first height in mm
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_h1, h1, "H1")


# -- H2
def get_q_resolution_h2():
    """
    Get the second height for rectangular apertures
    @returns the second height in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_h2, "H2")
    print(str(val))
    return val


def set_q_resolution_h2(h2):
    """
    Set the second height for rectangular apertures
    @param h2: the second height in mm
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_h2, h2, "H2")


# -- W1
def get_q_resolution_w1():
    """
    Get the first width for rectangular apertures
    @returns the first width in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_w1, "W1")
    print(str(val))
    return val


def set_q_resolution_w1(w1):
    """
    Set the first width for rectangular apertures
    @param w1: the first width in mm
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_w1, w1, "W1")


# -- W2
def get_q_resolution_w2():
    """
    Get the second width for rectangular apertures
    @returns the second width in mm
    """
    val = get_q_resolution_float(ReductionSingleton().to_Q.get_q_resolution_w2, "W2")
    print(str(val))
    return val


def set_q_resolution_w2(w2):
    """
    Set the second width for rectangular apertures
    @param w1: the second width in mm
    """
    set_q_resolution_float(ReductionSingleton().to_Q.set_q_resolution_w2, w2, "W2")


# -- Reset
def reset_q_resolution_settings():
    """
    Resets the q settings
    """
    ReductionSingleton().to_Q.reset_q_settings()


# -- Set float value
def set_q_resolution_float(func, arg, msg):
    """
    Set a q resolution value
    @param func: the specified function to run
    @param arg: the argument
    @param mgs: error message
    """
    if arg is None:
        return

    if su.is_convertible_to_float(arg):
        d_r = su.millimeter_2_meter(float(arg))
        func(d_r)
    else:
        sanslog.warning("Warning: Could not convert %s to float." % msg)


def get_q_resolution_float(func, msg):
    """
    Gets a q resolution value and checks if it has been set.
    @param func: the specified function to run
    @param mgs: error message
    @return the correct value
    """
    element = func()

    if su.is_convertible_to_float(element):
        element = su.meter_2_millimeter(element)
    elif not element:
        pass
    else:
        sanslog.warning("Warning: Could not convert %s to float." % msg)
    return element


def are_settings_consistent():
    """
    Runs the consistency check over all reductionssteps and reports cosistency
    issues to the user. The user needs to sort out these issues.
    """
    try:
        ReductionSingleton().perform_consistency_check()
    except RuntimeError as details:
        sanslog.error("There was an inconsistency issue with your settings. See details: %s" % str(details))
        raise RuntimeError("Please fix the following inconsistencies: %s" % str(details))


def is_current_workspace_an_angle_workspace():
    """
    Queries if the current workspace, stored in the reducer is a workspace
    which uses [angle, pos] to denote its location
    @returns true if it is an angle workspace else false
    """
    is_angle = False
    try:
        is_angle = is_workspace_which_requires_angle(reducer=ReductionSingleton())
    except (StopIteration, Exception, Warning):
        is_angle = False
    return is_angle


def _get_idf_path_for_run(file_name):
    """
    This method finds the full file location for a run number

    :param file_name: the file name or run number
    :return: the full path to the corresponding IDF
    """
    # Get measurement time from file
    measurement_time = su.get_measurement_time_from_file(file_name)

    # Get current instrument type
    instrument_name = ReductionSingleton().get_instrument_name()

    # Get the path to the instrument definition file
    idf_path_workspace = ExperimentInfo.getInstrumentFilename(instrument_name, measurement_time)
    return os.path.normpath(idf_path_workspace)


def get_idf_path_for_run(file_name):
    idf_path_workspace = _get_idf_path_for_run(file_name)
    print(idf_path_workspace)
    return idf_path_workspace


def MatchIDFInReducerAndWorkspace(file_name):
    """
    This method checks if the IDF which gets loaded with the workspace associated
    with the file name and the current instrument in the reducer singleton refer
    to the same IDF. If not then switch the IDF in the reducer.
    """

    # Get the IDF path
    idf_path_workspace = _get_idf_path_for_run(file_name)

    # Get the idf from the reducer
    idf_path_reducer = get_current_idf_path_in_reducer()

    is_matched = (idf_path_reducer == idf_path_workspace) and su.are_two_files_identical(idf_path_reducer, idf_path_reducer)

    return is_matched


def has_user_file_valid_extension(file_name):
    """
    Checks if the user file has a valid extension
    @param file_name: the name of the user file
    @returns true if it is valid else false
    """
    is_valid = su.is_valid_user_file_extension(file_name)
    print(str(is_valid))
    return is_valid


def get_current_idf_path_in_reducer():
    """
    This function returns the path to the IDF which is currently being used by the
    instrument stored in the reducer
    @returns a string with the path to the IDF which is currently being used.
    """
    idf_path_reducer = ReductionSingleton().get_idf_file_path()
    idf_path_reducer = os.path.normpath(idf_path_reducer)
    print(str(idf_path_reducer))
    return idf_path_reducer


##################### Accesor functions for BackgroundCorrection
def set_background_correction(run_number, is_time_based, is_mon, is_mean, mon_numbers=None):
    """
    Set a background correction setting.
    @param run_number: the run number
    @param is_time_based: if it is time-based or uamp-based
    @param is_mon: if it is a monitor or a detector
    @param is_mean: if it is mean or tof
    @param mon_numbers: the monitor numbers of interest or an empty string
    """

    def convert_from_comma_separated_string_to_int_list(input_string):
        """
        Convert from string with comma-separated values to a python int list
        @param input_string: the input string
        @returns an integer list
        @raises RuntimeError: conversion form string to int is not possible
        """
        if input_string is None or len(input_string) == 0:
            return None
        string_list = su.convert_to_list_of_strings(input_string)
        can_convert_to_int = all(su.is_convertible_to_int(element) for element in string_list)
        int_list = None
        if can_convert_to_int:
            int_list = [int(element) for element in string_list]
        else:
            raise RuntimeError("Cannot convert string list to integer list")
        return int_list

    mon_numbers_int = convert_from_comma_separated_string_to_int_list(mon_numbers)

    setting = UserFileParser.DarkRunSettings(
        run_number=run_number, time=is_time_based, mean=is_mean, mon=is_mon, mon_number=mon_numbers_int
    )
    ReductionSingleton().add_dark_run_setting(setting)


def get_background_correction(is_time, is_mon, component):
    """
    Gets the background corrections settings for a specific configuration
    This can be: time-based + detector, time_based + monitor,
                 uamp-based + detector, uamp_based + monitor
    @param is_time: is it time or uamp based
    @param is_mon: is it a monitor or a detector
    @param component: string with a component name (need to do this because of the python-C++ interface)
    """

    def convert_from_int_list_to_string(int_list):
        """
        Convert from a python list of integers to a string with comma-separated values
        @param int_list: the integer list
        @returns the string
        """
        if int_list is None or len(int_list) == 0:
            return None
        else:
            string_list = [str(element) for element in int_list]
            return su.convert_from_string_list(string_list)

    setting = ReductionSingleton().get_dark_run_setting(is_time, is_mon)

    value = None
    if setting is not None:
        if component == "run_number":
            value = setting.run_number
        elif component == "is_mean":
            value = str(setting.mean)
        elif component == "is_mon":
            value = str(setting.mon)
        elif component == "mon_number":
            value = convert_from_int_list_to_string(setting.mon_numbers)
        else:
            pass
    print(str(value))
    return value


def clear_background_correction():
    """
    Clears the background correction settings
    """
    ReductionSingleton().clear_dark_run_settings()


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
    rear_det = ReductionSingleton().instrument.getDetector("rear")
    rear_det.correction_file = filename


@deprecated
def SetFrontEfficiencyFile(filename):
    front_det = ReductionSingleton().instrument.getDetector("front")
    front_det.correction_file = filename


@deprecated
def displayUserFile():
    print("-- Mask file defaults --")
    print(ReductionSingleton().to_wavlen)
    print(ReductionSingleton().Q_string())
    #    print correction_files()
    print("    direct beam file rear:", end=" ")
    print(ReductionSingleton().instrument.detector_file("rear"))
    print("    direct beam file front:", end=" ")
    print(ReductionSingleton().instrument.detector_file("front"))
    print(ReductionSingleton().mask)


@deprecated
def displayMaskFile():
    displayUserFile()


@deprecated
def displayGeometry():
    [x, y] = ReductionSingleton().get_beam_center()
    print("Beam centre: [" + str(x) + "," + str(y) + "]")
    print(ReductionSingleton().get_sample().geometry)


@deprecated
def LimitsQ(*args):
    settings = ReductionSingleton().user_settings
    if settings is None:
        raise RuntimeError("MaskFile() first")

    # If given one argument it must be a rebin string
    if len(args) == 1:
        val = args[0]
        if isinstance(val, str):
            _printMessage("LimitsQ(" + val + ")")
            settings.readLimitValues("L/Q " + val, ReductionSingleton())
        else:
            issueWarning("LimitsQ can only be called with a single string or 4 values")
    elif len(args) == 4:
        qmin, qmax, step, step_type = args
        _printMessage("LimitsQ(" + str(qmin) + ", " + str(qmax) + ", " + str(step) + "," + str(step_type) + ")")
        settings.readLimitValues("L/Q " + str(qmin) + " " + str(qmax) + " " + str(step) + "/" + step_type, ReductionSingleton())
    else:
        issueWarning("LimitsQ called with " + str(len(args)) + " arguments, 1 or 4 expected.")


@deprecated
def ViewCurrentMask():
    """
    This opens InstrumentView to display the masked
    detectors in the bank in a different colour
    """
    raise NotImplementedError(
        "This is no longer implemented as it required MantidPlot, please switchto sans.command_interface.ISISCommandInterface instead"
    )


###############################################################################
########################## End of Deprecated Code #############################
########################################################################g#######

# this is like a #define I'd like to get rid of it because it seems meaningless here
DefaultTrans = "True"
NewTrans = "False"

_refresh_singleton()

if __name__ == "__main__":
    SetVerboseMode(True)
    SANS2D()
    MaskFile("MASKSANS2D_123T_4m_Xpress_8mm.txt")
    Set1D()
    AssignSample("SANS2D00002500.nxs")
    Gravity(True)
    wav1 = 2.0
    wav2 = wav1 + 2.0
    reduced = WavRangeReduction(wav1, wav2, DefaultTrans)
