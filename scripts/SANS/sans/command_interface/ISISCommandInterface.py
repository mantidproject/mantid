# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import inspect
import os
import re

import types

from SANSadd2 import add_runs
from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.kernel import config
from sans.command_interface.batch_csv_parser import BatchCsvParser
from sans.command_interface.command_interface_functions import print_message, warning_message
from sans.command_interface.command_interface_state_director import (
    CommandInterfaceStateDirector,
    DataCommand,
    DataCommandId,
    NParameterCommand,
    NParameterCommandId,
    FitData,
)
from sans.common.constants import ALL_PERIODS
from sans.common.enums import (
    DetectorType,
    FitType,
    RangeStepType,
    ReductionDimensionality,
    ReductionMode,
    SANSFacility,
    SaveType,
    OutputMode,
    FindDirectionEnum,
)
from sans.common.file_information import find_sans_file, find_full_file_path
from sans.common.general_functions import (
    convert_bank_name_to_detector_type_isis,
    get_output_name,
    is_part_of_reduced_output_workspace_group,
)
from sans.gui_logic.models.RowEntries import RowEntries
from sans.sans_batch import SANSBatchReduction, SANSCentreFinder


# ----------------------------------------------------------------------------------------------------------------------
# Globals
# ----------------------------------------------------------------------------------------------------------------------
DefaultTrans = True


# ----------------------------------------------------------------------------------------------------------------------
# CommandInterfaceStateDirector global instance
# ----------------------------------------------------------------------------------------------------------------------
director = CommandInterfaceStateDirector(SANSFacility.ISIS)
_plot_missing_str = "Plotting is not implemented for workbench yet, please contact us via the forum:\nhttps://forum.mantidproject.org/"


def deprecated(obj):
    """
    Decorator to apply to functions or classes that we think are not being (or
    should not be) used anymore.  Prints a warning to the log.
    """
    if inspect.isfunction(obj) or inspect.ismethod(obj):
        if inspect.isfunction(obj):
            obj_desc = '"%s" function' % obj.__name__
        else:
            obj_desc = '"%s" class' % obj.__self__.__class__.__name__

        def print_warning_wrapper(*args, **kwargs):
            warning_message(
                "The {0} has been marked as deprecated and may be "
                "removed in a future version of Mantid. If you "
                "believe this to have been marked in error, please "
                "contact the member of the Mantid team responsible "
                "for ISIS SANS.".format(obj_desc)
            )
            return obj(*args, **kwargs)

        return print_warning_wrapper

    # Add a @deprecated decorator to each of the member functions in the class
    # (by recursion).
    if inspect.isclass(obj):
        for name, fn in inspect.getmembers(obj):
            if isinstance(fn, types.MethodType):
                setattr(obj, name, deprecated(fn))
        return obj

    assert False, (
        "Programming error.  You have incorrectly applied the @deprecated decorator.  This is only for use with functions or classes."
    )


# ----------------------------------------------------------------------------------------------------------------------
# Unnecessary commands
# ----------------------------------------------------------------------------------------------------------------------
def SetVerboseMode(state):
    pass


# ----------------------------------------------------------------------------------------------------------------------
# Setting instruments
# ----------------------------------------------------------------------------------------------------------------------
def SANS2D(idf_path=None):
    config["default.instrument"] = "SANS2D"


def SANS2DTUBES():
    config["default.instrument"] = "SANS2D"


def LOQ(idf_path="LOQ_Definition_20020226-.xml"):
    config["default.instrument"] = "LOQ"


def LARMOR(idf_path=None):
    config["default.instrument"] = "LARMOR"


def ZOOM(idf_path=None):
    config["default.instrument"] = "ZOOM"


# ----------------------------------------------------------------------------------------------------------------------
# Unused commands
# ----------------------------------------------------------------------------------------------------------------------
@deprecated
def _SetWavelengthRange(start, end):
    _ = start
    _ = end
    pass


@deprecated
def Reduce():
    pass


@deprecated
def GetMismatchedDetList():
    pass


# ----------------------------------------------------------------------------------------------------------------------
# Currently not implemented commands
# ----------------------------------------------------------------------------------------------------------------------
def TransWorkspace(sample, can=None):
    """
    Use a given workpspace that contains pre-calculated transmissions
    @param sample the workspace to use for the sample
    @param can calculated transmission for the can
    """
    _, _ = sample, can
    raise NotImplementedError("The TransWorkspace command is not implemented in SANS v2.")


def createColetteScript(inputdata, format, reduced, centreit, plotresults, csvfile="", savepath=""):
    _, _, _, _, _, _, _ = inputdata, format, reduced, centreit, plotresults, csvfile, savepath
    raise NotImplementedError("The creatColleteScript command is not implemented in SANS v2.")


# ----------------------------------------------------------------------------------------------------------------------
# Data related commands
# ----------------------------------------------------------------------------------------------------------------------
def AssignSample(sample_run, reload=True, period=ALL_PERIODS):
    """
    Sets the sample scatter data.

    @param sample_run: run number to analysis e.g. SANS2D7777.nxs
    @param reload: must be set to True
    @param period: the period (entry) number to load, default is the first period
    """
    _ = reload
    # First of all the default for all periods used to be -1. If we encounter this then set periods to ALL_PERIODS
    period = int(period)
    period = ALL_PERIODS if period == -1 else period

    # Print the output
    message = 'AssignSample("' + str(sample_run) + '"'
    if period != ALL_PERIODS:
        message += ", " + str(period)
    message += ")"
    print_message(message)

    # Get the full file name of the run
    file_name = find_sans_file(sample_run)

    # Set the command
    data_command = DataCommand(command_id=DataCommandId.SAMPLE_SCATTER, file_name=file_name, period=period)
    director.add_command(data_command)


def AssignCan(can_run, reload=True, period=ALL_PERIODS):
    """
    Sets the can scatter data.

    @param can_run: run number to analysis e.g. SANS2D7777.nxs
    @param reload: must be set to True
    @param period: the period (entry) number to load, default is the first period
    """
    _ = reload
    # First of all the default for all periods used to be -1. If we encounter this then set periods to ALL_PERIODS
    period = int(period)
    period = ALL_PERIODS if period == -1 else period

    # Print the output
    message = 'AssignCan("' + str(can_run) + '"'
    if period != ALL_PERIODS:
        message += ", " + str(period)
    message += ")"
    print_message(message)

    # Get the full file name of the run
    file_name = find_sans_file(can_run)

    # Set the command
    data_command = DataCommand(command_id=DataCommandId.CAN_SCATTER, file_name=file_name, period=period)
    director.add_command(data_command)


def TransmissionSample(sample, direct, reload=True, period_t=ALL_PERIODS, period_d=ALL_PERIODS):
    """
    Specify the transmission and direct runs for the sample.

    @param sample: the transmission run
    @param direct: direct run
    @param reload: if to replace the workspace if it is already there
    @param period_t: the entry number of the transmission run (default single entry file)
    @param period_d: the entry number of the direct run (default single entry file)
    """
    _ = reload
    # First of all the default for all periods used to be -1. If we encounter this then set periods to ALL_PERIODS
    period_t = int(period_t)
    period_d = int(period_d)
    period_t = ALL_PERIODS if period_t == -1 else period_t
    period_d = ALL_PERIODS if period_d == -1 else period_d

    print_message('TransmissionSample("' + str(sample) + '","' + str(direct) + '")')

    # Get the full file name of the run
    trans_file_name = find_sans_file(sample)
    direct_file_name = find_sans_file(direct)

    # Set the command
    trans_command = DataCommand(command_id=DataCommandId.SAMPLE_TRANSMISSION, file_name=trans_file_name, period=period_t)
    direct_command = DataCommand(command_id=DataCommandId.SAMPLE_DIRECT, file_name=direct_file_name, period=period_d)
    director.add_command(trans_command)
    director.add_command(direct_command)


def TransmissionCan(can, direct, reload=True, period_t=-1, period_d=-1):
    """
    Specify the transmission and direct runs for the can
    @param can: the transmission run
    @param direct: direct run
    @param reload: if to replace the workspace if it is already there
    @param period_t: the entry number of the transmission run (default single entry file)
    @param period_d: the entry number of the direct run (default single entry file)
    """
    _ = reload
    # First of all the default for all periods used to be -1. If we encounter this then set periods to ALL_PERIODS
    period_t = int(period_t)
    period_d = int(period_d)
    period_t = ALL_PERIODS if period_t == -1 else period_t
    period_d = ALL_PERIODS if period_d == -1 else period_d

    print_message('TransmissionCan("' + str(can) + '","' + str(direct) + '")')

    # Get the full file name of the run
    trans_file_name = find_sans_file(can)
    direct_file_name = find_sans_file(direct)

    # Set the command
    trans_command = DataCommand(command_id=DataCommandId.CAN_TRANSMISSION, file_name=trans_file_name, period=period_t)
    direct_command = DataCommand(command_id=DataCommandId.CAN_DIRECT, file_name=direct_file_name, period=period_d)
    director.add_command(trans_command)
    director.add_command(direct_command)


# ----------------------------------------------------------------------------------------------------------------------
# N parameter commands
# ----------------------------------------------------------------------------------------------------------------------


# ------------------------
# Zero parameters
# ------------------------
def Clean():
    """
    Removes all previous settings.
    """
    clean_command = NParameterCommand(command_id=NParameterCommandId.CLEAN, values=[])
    director.add_command(clean_command)


def Set1D():
    """
    Sets the reduction dimensionality to 1D
    """
    print_message("Set1D()")
    set_1d_command = NParameterCommand(command_id=NParameterCommandId.REDUCTION_DIMENSIONALITY, values=[ReductionDimensionality.ONE_DIM])
    director.add_command(set_1d_command)


def Set2D():
    """
    Sets the reduction dimensionality to 2D
    """
    print_message("Set2D()")
    set_2d_command = NParameterCommand(command_id=NParameterCommandId.REDUCTION_DIMENSIONALITY, values=[ReductionDimensionality.TWO_DIM])
    director.add_command(set_2d_command)


def UseCompatibilityMode():
    """
    Sets the compatibility mode to True
    """
    set_2d_command = NParameterCommand(command_id=NParameterCommandId.COMPATIBILITY_MODE, values=[True])
    director.add_command(set_2d_command)


# -------------------------
# Single parameter commands
# -------------------------
def MaskFile(file_name):
    """
    Loads the user file (note that mask file is the legacy description user file)

    @param file_name: path to the user file.
    """
    if not file_name:
        raise ValueError("An empty filename was passed to MaskFile")

    file_path = file_name if os.path.exists(file_name) else find_full_file_path(file_name)

    if not file_path or not os.path.isfile(file_path):
        raise FileNotFoundError("Could not find MaskFile: {0}".format(file_name))

    print_message('#Opening "' + file_path + '"')
    user_file_command = NParameterCommand(command_id=NParameterCommandId.USER_FILE, values=[file_path])
    director.add_command(user_file_command)


def Mask(details):
    """
    Allows the user to specify a mask command as is done in the user file.

    @param details: a string that specifies masking as it would appear in a mask file
    """
    print_message('Mask("' + details + '")')
    mask_command = NParameterCommand(command_id=NParameterCommandId.MASK, values=[details])
    director.add_command(mask_command)


def SetSampleOffset(value):
    """
    Set the sample offset.

    @param value: the offset in mm
    """
    value = float(value)
    sample_offset_command = NParameterCommand(command_id=NParameterCommandId.SAMPLE_OFFSET, values=[value])
    director.add_command(sample_offset_command)


def Detector(det_name):
    """
    Sets the detector which is being used for the reduction.

    Previous comment: Sets the detector bank to use for the reduction e.g. 'front-detector'. The main detector is
     assumed if this line is not given
    @param det_name: the detector's name
    """
    print_message('Detector("' + det_name + '")')
    detector_type = convert_bank_name_to_detector_type_isis(det_name)
    reduction_mode = ReductionMode.HAB if detector_type is DetectorType.HAB else ReductionMode.LAB
    detector_command = NParameterCommand(command_id=NParameterCommandId.DETECTOR, values=[reduction_mode])
    director.add_command(detector_command)


def SetEventSlices(input_str):
    """
    Sets the events slices
    """
    event_slices_command = NParameterCommand(command_id=NParameterCommandId.EVENT_SLICES, values=input_str)
    director.add_command(event_slices_command)


# ----------------------------------------------------------------------------------------------------------------------
# Double valued commands
# ----------------------------------------------------------------------------------------------------------------------
def SetMonitorSpectrum(specNum, interp=False):
    """
    Specifies the spectrum number of the spectrum that will be used to for monitor normalisation
    @param specNum: a spectrum number (1 or greater)
    @param interp: when rebinning the wavelength bins to match the main workspace, if use interpolation
                   default no interpolation
    """
    specNum = int(specNum)
    is_trans = False
    monitor_spectrum_command = NParameterCommand(command_id=NParameterCommandId.INCIDENT_SPECTRUM, values=[specNum, interp, is_trans])
    director.add_command(monitor_spectrum_command)


def SetTransSpectrum(specNum, interp=False):
    """
    Sets the spectrum number (of the incident monitor) and the interpolation configuration for transmission calculation.

    @param specNum: a spectrum number (1 or greater)
    @param interp: when rebinning the wavelength bins to match the main workspace, if use interpolation
                   default no interpolation
    """
    specNum = int(specNum)
    is_trans = True
    transmission_spectrum_command = NParameterCommand(command_id=NParameterCommandId.INCIDENT_SPECTRUM, values=[specNum, interp, is_trans])
    director.add_command(transmission_spectrum_command)


def Gravity(flag, extra_length=0.0):
    """
    Allows the user to set the gravity correction for the q conversion.
    @param flag: set to True if the correction should be used, else False.
    @param extra_length: the extra length in meter.
    @return:
    """
    extra_length = float(extra_length)
    print_message("Gravity(" + str(flag) + ", " + str(extra_length) + ")")
    gravity_command = NParameterCommand(command_id=NParameterCommandId.GRAVITY, values=[flag, extra_length])
    director.add_command(gravity_command)


def SetDetectorFloodFile(filename, detector_name="REAR"):
    """
    Sets the pixel correction file for a particular detector

    @param filename: the name of the file.
    @param detector_name: the name of the detector
    """
    file_name = find_full_file_path(filename)
    detector_name = convert_bank_name_to_detector_type_isis(detector_name)
    flood_command = NParameterCommand(command_id=NParameterCommandId.FLOOD_FILE, values=[file_name, detector_name])
    director.add_command(flood_command)


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
    print_message("SetCorrectionFile(" + str(bank) + ", " + filename + ")")
    detector_type = convert_bank_name_to_detector_type_isis(bank)
    file_name = find_full_file_path(filename)
    flood_command = NParameterCommand(command_id=NParameterCommandId.WAVELENGTH_CORRECTION_FILE, values=[file_name, detector_type])
    director.add_command(flood_command)


# --------------------------
# Three parameter commands
# ---------------------------
def SetCentre(xcoord, ycoord, bank="rear"):
    """
    Configure the Beam Center position. It support the configuration of the centre for
    both detectors bank (low-angle bank and high-angle bank detectors)

    It allows defining the position for both detector banks.
    :param xcoord: X position of beam center in the user coordinate system.
    :param ycoord: Y position of beam center in the user coordinate system.
    :param bank: The selected bank ('rear' - low angle or 'front' - high angle)
    Introduced #5942
    """
    xcoord = float(xcoord)
    ycoord = float(ycoord)
    print_message("SetCentre(" + str(xcoord) + ", " + str(ycoord) + ")")
    detector_type = convert_bank_name_to_detector_type_isis(bank)
    centre_command = NParameterCommand(command_id=NParameterCommandId.CENTRE, values=[xcoord, ycoord, detector_type])
    director.add_command(centre_command)


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
    print_message("SetPhiLimit(" + str(phimin) + ", " + str(phimax) + ",use_mirror=" + str(use_mirror) + ")")
    # a beam centre of [0,0,0] makes sense if the detector has been moved such that beam centre is at [0,0,0]
    phimin = float(phimin)
    phimax = float(phimax)
    centre_command = NParameterCommand(command_id=NParameterCommandId.PHI_LIMIT, values=[phimin, phimax, use_mirror])
    director.add_command(centre_command)


def set_save(save_algorithms, save_as_zero_error_free):
    """
    Mainly internally used by BatchMode. Provides the save settings.

    @param save_algorithms: A list of SaveType enums.
    @param save_as_zero_error_free: True if a zero error correction should be performed.
    """
    save_command = NParameterCommand(command_id=NParameterCommandId.SAVE, values=[save_algorithms, save_as_zero_error_free])
    director.add_command(save_command)


# --------------------------
# Four parameter commands
# ---------------------------
def TransFit(mode, lambdamin=None, lambdamax=None, selector="BOTH"):
    """
    Sets the fit method to calculate the transmission fit and the wavelength range
    over which to do the fit. These arguments are passed to the algorithm
    CalculateTransmission. If mode is set to 'Off' then the unfitted workspace is
    used and lambdamin and max have no effect
    @param mode: can be 'Logarithmic' ('YLOG', 'LOG') 'OFF' ('CLEAR') or 'LINEAR' (STRAIGHT', LIN'),
                 'POLYNOMIAL2', 'POLYNOMIAL3', ...
    @param lambdamin: the lowest wavelength to use in any fit
    @param lambdamax: the end of the fit range
    @param selector: define for which transmission this fit specification is valid (BOTH, SAMPLE, CAN)
    """

    def does_pattern_match(compiled_regex, line):
        return compiled_regex.match(line) is not None

    def extract_polynomial_order(line):
        order = re.sub("POLYNOMIAL", "", line)
        order = order.strip()
        return int(order)

    polynomial_pattern = re.compile("\\s*" + "POLYNOMIAL" + "\\s*[2-9]")
    polynomial_order = None
    # Get the fit mode
    mode = str(mode).strip().upper()

    if mode == "LINEAR" or mode == "STRAIGHT" or mode == "LIN":
        fit_type = FitType.LINEAR
    elif mode == "LOGARITHMIC" or mode == "LOG" or mode == "YLOG":
        fit_type = FitType.LOGARITHMIC
    elif does_pattern_match(polynomial_pattern, mode):
        fit_type = FitType.POLYNOMIAL
        polynomial_order = extract_polynomial_order(mode)
    else:
        fit_type = FitType.NO_FIT

    # Get the selected detector to which the fit settings apply
    selector = str(selector).strip().upper()
    if selector == "SAMPLE":
        fit_data = FitData.Sample
    elif selector == "CAN":
        fit_data = FitData.Can
    elif selector == "BOTH":
        fit_data = FitData.Both
    else:
        raise RuntimeError("TransFit: The selected fit data {0} is not valid. You have to either SAMPLE, CAN or BOTH.".format(selector))

    # Output message
    message = mode
    if lambdamin:
        lambdamin = float(lambdamin)
        message += ", " + str(lambdamin)
    if lambdamax:
        lambdamax = float(lambdamax)
        message += ", " + str(lambdamax)
    message += ", selector=" + selector
    print_message('TransFit("' + message + '")')

    # Configure fit settings
    polynomial_order = polynomial_order if polynomial_order is not None else 0
    fit_command = NParameterCommand(
        command_id=NParameterCommandId.TRANS_FIT, values=[fit_data, lambdamin, lambdamax, fit_type, polynomial_order]
    )
    director.add_command(fit_command)


def LimitsR(rmin, rmax, quiet=False, reducer=None):
    """
    Sets the radius limits

    @param rmin: minimal radius in mm
    @param rmax: maximal radius in mm
    @param quiet: if True then no message will be logged.
    @param reducer: legacy parameter
    """
    _ = reducer
    rmin = float(rmin)
    rmax = float(rmax)
    if not quiet:
        print_message("LimitsR(" + str(rmin) + ", " + str(rmax) + ")", reducer)
    rmin /= 1000.0
    rmax /= 1000.0
    radius_command = NParameterCommand(command_id=NParameterCommandId.MASK_RADIUS, values=[rmin, rmax])
    director.add_command(radius_command)


def LimitsWav(lmin, lmax, step, bin_type):
    """
    Set the wavelength limits

    @param lmin: the lower wavelength bound.
    @param lmax: the upper wavelength bound.
    @param step: the wavelength step.
    @param bin_type: the bin type, ie linear or logarithmic. Accepted strings are "LINEAR" and "LOGARITHMIC"
    """
    lmin = float(lmin)
    lmax = float(lmax)
    step = float(step)

    print_message("LimitsWav(" + str(lmin) + ", " + str(lmax) + ", " + str(step) + ", " + bin_type + ")")

    rebin_string = bin_type.strip().upper()
    rebin_type = RangeStepType.LOG if rebin_string == "LOGARITHMIC" else RangeStepType.LIN

    wavelength_command = NParameterCommand(command_id=NParameterCommandId.WAVELENGTH_LIMIT, values=[lmin, lmax, step, rebin_type])
    director.add_command(wavelength_command)


def LimitsQXY(qmin, qmax, step):
    """
    To set the bin parameters for the algorithm Qxy()
    @param qmin: the first Q value to include
    @param qmaz: the last Q value to include
    @param step: bin width
    """
    qmin = float(qmin)
    qmax = float(qmax)
    step = float(step)

    print_message("LimitsQXY(" + str(qmin) + ", " + str(qmax) + ", " + str(step) + ")")
    qxy_command = NParameterCommand(command_id=NParameterCommandId.QXY_LIMIT, values=[qmin, qmax, step])
    director.add_command(qxy_command)


# --------------------------
# Six parameter commands
# --------------------------
def SetFrontDetRescaleShift(scale=1.0, shift=0.0, fitScale=False, fitShift=False, qMin=None, qMax=None):
    """
    Stores property about the detector which is used to rescale and shift
    data in the bank after data have been reduced
    @param scale: Default to 1.0. Value to multiply data with
    @param shift: Default to 0.0. Value to add to data
    @param fitScale: Default is False. Whether or not to try and fit this param
    @param fitShift: Default is False. Whether or not to try and fit this param
    @param qMin: When set to None (default) then for fitting use the overlapping q region of
                 front and rear detectors
    @param qMax: When set to None (default) then for fitting use the overlapping q region of
                 front and rear detectors
    """
    scale = float(scale)
    shift = float(shift)

    if qMin:
        qMin = float(qMin)
    if qMax:
        qMax = float(qMax)

    print_message("Set front detector rescale/shift values to {0} and {1}".format(scale, shift))
    front_command = NParameterCommand(
        command_id=NParameterCommandId.FRONT_DETECTOR_RESCALE, values=[scale, shift, fitScale, fitShift, qMin, qMax]
    )
    director.add_command(front_command)


def SetDetectorOffsets(bank, x, y, z, rot, radius, side, xtilt=0.0, ytilt=0.0):
    """
    Adjust detector position away from position defined in IDF. On SANS2D the detector
    banks can be moved around. This method allows fine adjustments of detector bank position
    in the same way as the DET/CORR userfile command works. Hence please see
    http://www.mantidproject.org/SANS_User_File_Commands#DET for details.

    The comment below is not true any longer:
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
    @param xtilt: xtilt in degrees
    @param ytilt: ytilt in degrees
    """
    x = float(x)
    y = float(y)
    z = float(z)
    rot = float(rot)
    radius = float(radius)
    side = float(side)
    xtilt = float(xtilt)
    ytilt = float(ytilt)

    print_message(
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
    detector_type = convert_bank_name_to_detector_type_isis(bank)
    detector_offsets = NParameterCommand(
        command_id=NParameterCommandId.DETECTOR_OFFSETS, values=[detector_type, x, y, z, rot, radius, side, xtilt, ytilt]
    )
    director.add_command(detector_offsets)


def _validate_wavrange_types(start, end):
    def is_set(val):
        return val is not None

    def is_list(val):
        return is_set(val) and isinstance(val, list)

    # If one input is a list, they must both be lists of the same length
    if is_list(start) != is_list(end):
        raise RuntimeError(
            "WavRangeReduction: The wav_start and wav_end inputs must both be the same type (got a mixture of single value and list)"
        )
    if is_list(start) and is_list(end) and len(start) != len(end):
        raise RuntimeError("WavRangeReduction: the wav_start and wav_end inputs must contain the same number of values")

    # Ensure values are stored as floats
    if is_list(start):
        start = [float(wav) for wav in start]
    elif is_set(start):
        start = float(start)

    if is_list(end):
        end = [float(wav) for wav in end]
    elif is_set(end):
        end = float(end)

    return start, end


# --------------------------------------------
# Commands which actually kick off a reduction
# --------------------------------------------
def WavRangeReduction(
    wav_start=None,
    wav_end=None,
    full_trans_wav=None,
    name_suffix=None,
    combineDet=None,
    resetSetup=True,
    out_fit_settings=None,
    output_name=None,
    output_mode=OutputMode.PUBLISH_TO_ADS,
    use_reduction_mode_as_suffix=False,
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
                       'front'               (run one reduction for the 'front' detector data, and
                                              rescale+shift 'front' data)
                       'both'                (run both the above two reductions)
                       'merged'              (run the same reductions as 'both' and additionally create
                                              a merged data workspace)
                        None                 (run one reduction for whatever detector has been set as the
                                              current detector
                                              before running this method. If front apply rescale+shift)
    @param resetSetup: if true reset setup at the end
    @param out_fit_settings: An output parameter. It is used, specially when resetSetup is True, in order
                             to remember the 'scale and fit' of the fitting algorithm.
    @param output_name: name of the output workspace/file, if none is specified then one is generated internally.
    @param output_mode: the way the data should be put out: Can be PublishToADS, SaveToFile or Both
    @param use_reduction_mode_as_suffix: If true then a second suffix will be used which is
                                         based on the reduction mode.
    @return Name of one of the workspaces created
    """
    print_message("WavRangeReduction(" + str(wav_start) + ", " + str(wav_end) + ", " + str(full_trans_wav) + ")")
    _ = resetSetup
    _ = out_fit_settings

    # Set the provided parameters
    if combineDet is None:
        reduction_mode = None
    elif combineDet == "rear":
        reduction_mode = ReductionMode.LAB
    elif combineDet == "front":
        reduction_mode = ReductionMode.HAB
    elif combineDet == "merged":
        reduction_mode = ReductionMode.MERGED
    elif combineDet == "both":
        reduction_mode = ReductionMode.ALL
    else:
        raise RuntimeError(
            "WavRangeReduction: The combineDet input parameter was given a value of {0}. rear, front,"
            " both, merged and no input are allowed".format(combineDet)
        )

    wav_start, wav_end = _validate_wavrange_types(wav_start, wav_end)

    wavelength_command = NParameterCommand(
        command_id=NParameterCommandId.WAV_RANGE_SETTINGS, values=[wav_start, wav_end, full_trans_wav, reduction_mode]
    )
    director.add_command(wavelength_command)

    # Save options
    if output_name is not None:
        director.add_command(NParameterCommand(command_id=NParameterCommandId.USER_SPECIFIED_OUTPUT_NAME, values=[output_name]))
    if name_suffix is not None:
        director.add_command(NParameterCommand(command_id=NParameterCommandId.USER_SPECIFIED_OUTPUT_NAME_SUFFIX, values=[name_suffix]))
    if use_reduction_mode_as_suffix:
        director.add_command(
            NParameterCommand(command_id=NParameterCommandId.USE_REDUCTION_MODE_AS_SUFFIX, values=[use_reduction_mode_as_suffix])
        )

    # Get the states
    state = director.process_commands()

    # Run the reduction
    batch_alg = SANSBatchReduction()
    batch_alg(states=[state], use_optimizations=True, output_mode=output_mode)

    # -----------------------------------------------------------
    # Return the name fo the reduced workspace (or WorkspaceGroup)
    # -----------------------------------------------------------
    reduction_mode = state.reduction.reduction_mode
    is_group = is_part_of_reduced_output_workspace_group(state)
    wav_range = state.wavelength.wavelength_interval.wavelength_full_range
    if reduction_mode != ReductionMode.ALL:
        _, output_workspace_base_name = get_output_name(state, reduction_mode, is_group, wav_range)
    else:
        _, output_workspace_base_name_hab = get_output_name(state, ReductionMode.HAB, is_group, wav_range)
        _, output_workspace_base_name_lab = get_output_name(state, ReductionMode.LAB, is_group, wav_range)
        output_workspace_base_name = [output_workspace_base_name_lab, output_workspace_base_name_hab]
    return output_workspace_base_name


def BatchReduce(  # noqa
    filename,
    format,
    plotresults=False,
    saveAlgs=None,
    verbose=False,
    centreit=False,
    reducer=None,
    combineDet=None,
    save_as_zero_error_free=False,
):
    """
    @param filename: the CSV file with the list of runs to analyse
    @param format: type of file to load, nxs for Nexus, etc.
    @param plotresults: if true and this function is run from mantid a graph will be created for the results of each reduction
    @param saveAlgs: this named algorithm will be passed the name of the results workspace and filename (default = 'SaveRKH').
        Pass a tuple of strings to save to multiple file formats
    @param verbose: set to true to write more information to the log (default=False)
    @param centreit: do centre finding (default=False)
    @param reducer: if to use the command line (default) or GUI reducer object
    @param combineDet: that will be forward to WavRangeReduction (rear, front, both, merged, None)
    @param save_as_zero_error_free: Should the reduced workspaces contain zero errors or not
    @return final_setings: A dictionary with some values of the Reduction - Right Now:(scale, shift)
    """
    if saveAlgs is None:
        saveAlgs = {"SaveRKH": "txt"}

    # From the old interface
    _ = format
    _ = reducer
    _ = verbose

    if centreit:
        raise RuntimeError("The beam centre finder is currently not supported.")
    if plotresults:
        raise RuntimeError("Plotting the results is currently not supported.")

    # Set up the save algorithms
    save_algs = []

    if saveAlgs:
        for key, _ in list(saveAlgs.items()):
            if key == "SaveRKH":
                save_algs.append(SaveType.RKH)
            elif key == "SaveNexus":
                save_algs.append(SaveType.NEXUS)
            elif key == "SaveNistQxy":
                save_algs.append(SaveType.NIST_QXY)
            elif key == "SaveCanSAS" or key == "SaveCanSAS1D":
                save_algs.append(SaveType.CAN_SAS)
            elif key == "SaveCSV":
                save_algs.append(SaveType.CSV)
            elif key == "SaveNXcanSAS":
                save_algs.append(SaveType.NX_CAN_SAS)
            else:
                raise RuntimeError("The save format {0} is not known.".format(key))
        output_mode = OutputMode.BOTH
    else:
        output_mode = OutputMode.PUBLISH_TO_ADS

    # Get the information from the csv file
    batch_csv_parser = BatchCsvParser()
    parsed_batch_entries = batch_csv_parser.parse_batch_file(filename)

    # Get a state with all existing settings
    for parsed_batch_entry in parsed_batch_entries:
        assert isinstance(parsed_batch_entry, RowEntries)
        # A new user file. If a new user file is provided then this will overwrite all other settings from,
        # otherwise we might have cross-talk between user files.
        if parsed_batch_entry.user_file:
            MaskFile(parsed_batch_entry.user_file)

        # Sample scatter
        sample_scatter = parsed_batch_entry.sample_scatter
        sample_scatter_period = parsed_batch_entry.sample_scatter_period
        AssignSample(sample_run=sample_scatter, period=sample_scatter_period)

        # Sample transmission
        if parsed_batch_entry.sample_transmission and parsed_batch_entry.sample_direct:
            sample_direct = parsed_batch_entry.sample_direct
            sample_direct_period = parsed_batch_entry.sample_direct_period

            sample_transmission = parsed_batch_entry.sample_transmission
            sample_transmission_period = parsed_batch_entry.sample_transmission_period

            TransmissionSample(
                sample=sample_transmission, direct=sample_direct, period_t=sample_transmission_period, period_d=sample_direct_period
            )

        # Can scatter
        if parsed_batch_entry.can_scatter:
            can_scatter = parsed_batch_entry.can_scatter
            can_scatter_period = parsed_batch_entry.can_scatter_period
            AssignCan(can_run=can_scatter, period=can_scatter_period)

        # Can transmission
        if parsed_batch_entry.can_transmission and parsed_batch_entry.can_direct:
            can_transmission = parsed_batch_entry.can_transmission
            can_transmission_period = parsed_batch_entry.can_transmission_period
            can_direct = parsed_batch_entry.can_direct
            can_direct_period = parsed_batch_entry.can_direct_period

            TransmissionCan(can=can_transmission, direct=can_direct, period_t=can_transmission_period, period_d=can_direct_period)

        # Name of the output. We need to modify the name according to the setup of the old reduction mechanism
        output_name = parsed_batch_entry.output_name

        # In addition to the output name the user can specify with combineDet an additional suffix (in addition to the
        # suffix that the user can set already -- was there previously, so we have to provide that)
        use_reduction_mode_as_suffix = combineDet is not None

        # Apply save options
        if save_algs:
            set_save(save_algorithms=save_algs, save_as_zero_error_free=save_as_zero_error_free)

        # Run the reduction for a single
        reduced_workspace_name = WavRangeReduction(
            combineDet=combineDet,
            output_name=output_name,
            output_mode=output_mode,
            use_reduction_mode_as_suffix=use_reduction_mode_as_suffix,
        )

        # Remove the settings which were very specific for this single reduction which are:
        # 1. The last user file (if any was set)
        # 2. The last scatter entry
        # 3. The last scatter transmission and direct entry (if any were set)
        # 4. The last can scatter ( if any was set)
        # 5. The last can transmission and direct entry (if any were set)
        if parsed_batch_entry.user_file:
            director.remove_last_user_file()

        director.remove_last_scatter_sample()

        if parsed_batch_entry.sample_transmission and parsed_batch_entry.sample_direct:
            director.remove_last_sample_transmission_and_direct()

        if parsed_batch_entry.can_scatter:
            director.remove_last_scatter_can()

        if parsed_batch_entry.can_transmission and parsed_batch_entry.can_direct:
            director.remove_last_can_transmission_and_direct()

        # Plot the results if that was requested, the flag 1 is from the old version.
        if plotresults == 1:
            if AnalysisDataService.doesExist(reduced_workspace_name):
                workspace = AnalysisDataService.retrieve(reduced_workspace_name)
                if isinstance(workspace, WorkspaceGroup):
                    for ws in workspace:
                        PlotResult(ws.name())
                else:
                    PlotResult(workspace.name())


def CompWavRanges(wavelens, plot=True, combineDet=None, resetSetup=True):
    """
    Compares the momentum transfer results calculated from different wavelength ranges. Given
    the list of wave ranges [a, b, c] it reduces for wavelengths a-b, b-c and a-c.
    @param wavelens: the list of wavelength ranges
    @param plot: set this to true to plot the result (must be run in Mantid), default is true
    @param combineDet: see description in WavRangeReduction
    @param resetSetup: if true reset setup at the end
    """

    print_message("CompWavRanges( %s,plot=%s)" % (str(wavelens), plot))

    if not isinstance(wavelens, list) or len(wavelens) < 2:
        if not isinstance(wavelens, tuple):
            raise RuntimeError("Error CompWavRanges() requires a list of wavelengths between which reductions will be performed.")

    # Perform a reduction over the full wavelength range which was specified
    reduced_workspace_names = []

    for index in range(len(wavelens)):
        wavelens[index] = float(wavelens[index])

    full_reduction_name = WavRangeReduction(wav_start=wavelens[0], wav_end=wavelens[-1], combineDet=combineDet, resetSetup=False)
    reduced_workspace_names.append(full_reduction_name)

    # Reduce each wavelength slice
    for i in range(0, len(wavelens) - 1):
        reduced_workspace_name = WavRangeReduction(wav_start=wavelens[i], wav_end=wavelens[i + 1], combineDet=combineDet, resetSetup=False)
        reduced_workspace_names.append(reduced_workspace_name)

    if plot:
        raise NotImplementedError(_plot_missing_str)

    # Return just the workspace name of the full range
    return reduced_workspace_names[0]


def PhiRanges(phis, plot=True):
    """
    Given a list of phi ranges [a, b, c, d] it reduces in the phi ranges a-b and c-d
    @param phis: the list of phi ranges
    @param plot: set this to true to plot the result (must be run in Mantid), default is true
    """

    print_message("PhiRanges( %s,plot=%s)" % (str(phis), plot))

    # todo covert their string into Python array

    if len(phis) % 2 != 0:
        raise RuntimeError("Phi ranges must be given as pairs")

    reduced_workspace_names = []
    for i in range(0, len(phis), 2):
        SetPhiLimit(phis[i], phis[i + 1])
        reduced_workspace_name = WavRangeReduction()
        reduced_workspace_names.append(reduced_workspace_name)

    if plot:
        raise NotImplementedError(_plot_missing_str)

    # Return just the workspace name of the full range
    return reduced_workspace_names[0]


def FindBeamCentre(
    rlow, rupp, MaxIter=10, xstart=None, ystart=None, tolerance=1.251e-4, find_direction=FindDirectionEnum.ALL, reduction_method=True
):
    state = director.process_commands()
    """
    Finds the beam centre position.
    @param rlow: Inner radius of quadrant
    @param rupp: Outer radius of quadrant
    @param MaxIter: Maximum number of iterations
    @param xstart: starting x centre position, if not set is taken from user file
    @param ystart: starting y centre position, if not set is taken from user file
    @param tolerance: Sets the step size at which the search will stop
    @param find_direction: FindDirectionEnum controls which directions to find the centre in by default is All
    @param reduction_method: if true uses the quadrant centre finder method. If false user the COM method
    """

    if not xstart:
        xstart = state.move.detectors["LAB"].sample_centre_pos1
    elif config["default.instrument"] == "LARMOR":
        # This is to maintain compatibility with how this function worked in the old Interface so that legacy scripts still
        # function
        xstart = xstart * 1000
    if not ystart:
        ystart = state.move.detectors["LAB"].sample_centre_pos2

    centre_finder = SANSCentreFinder()
    centre = centre_finder(
        state, float(rlow), float(rupp), MaxIter, float(xstart), float(ystart), float(tolerance), find_direction, reduction_method
    )

    if config["default.instrument"] == "LARMOR":
        SetCentre(centre["pos1"], 1000 * centre["pos2"], bank="rear")
    else:
        SetCentre(1000 * centre["pos1"], 1000 * centre["pos2"], bank="rear")
    if "HAB" in state.move.detectors:
        SetCentre(1000 * centre["pos1"], 1000 * centre["pos2"], bank="front")

    return centre


# ----------------------------------------------------------------------------------------------------------------------
# General commands
# ----------------------------------------------------------------------------------------------------------------------
def PlotResult(workspace, canvas=None):
    """
    Draws a graph of the passed workspace. If the workspace is 2D (has many spectra
    a contour plot is written
    @param workspace: a workspace name or handle to plot
    @param canvas: optional handle to an existing graph to write the plot to
    @return: a handle to the graph that was written to
    """
    raise NotImplementedError(_plot_missing_str)

    try:
        import mantidplot

        workspace = AnalysisDataService.retrieve(str(workspace))
        number_of_spectra = workspace[0].getNumberHistograms() if isinstance(workspace, WorkspaceGroup) else workspace.getNumberHistograms()
        graph = (
            mantidplot.plotSpectrum(workspace, 0)
            if number_of_spectra == 1
            else mantidplot.importMatrixWorkspace(workspace.name()).plotGraph2D()
        )

        if canvas is not None:
            # we were given a handle to an existing graph, use it
            mantidplot.mergePlots(canvas, graph)
            graph = canvas
        return graph
    except ImportError:
        print_message("Plot functions are not available, is this being run from outside Mantidplot?")


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
                        in a string list. Either time_shifts is not used or a list with times in seconds. Note that there
                        has to be one entry fewer than the number of workspaces to add.
    @param defType: the file type
    @param rawTypes: the raw types
    @param lowMem: if the lowMem option should be used
    @returns a success message
    """
    # Need at least two runs to work
    if len(runs) < 1:
        print_message("AddRuns issue: A list with at least two runs needs to be provided.")
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
