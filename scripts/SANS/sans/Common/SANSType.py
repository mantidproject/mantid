""" The elements of this module define types which are used in the SANS reduction framework."""

# pylint: disable=too-few-public-methods, invalid-name

from SANS2.Common.SANSConstants import SANSConstants


# -------------------------------------------
# SANS Type decorator
# -------------------------------------------
def sans_type(*inner_classes):
    """
    Class decorator which changes the name of an inner class to include the name of the outer class. The inner class
    gets a method to determine the name of the outer class. This information is needed for serialization at the
    algorithm input boundary.
    """
    def inner_class_builder(cls):
        # Add each inner class to the outer class
        for inner_class in inner_classes:
            new_class = type(inner_class, (cls, ), {"outer_class_name": cls.__name__})
            # We set the module of the inner class to the module of the outer class. We have to do this since we
            # are dynamically adding the inner class which gets its module name from the module where it was added,
            # but not where the outer class lives.
            module_of_outer_class = getattr(cls, "__module__")
            setattr(new_class, "__module__", module_of_outer_class)
            # Add the inner class to the outer class
            setattr(cls, inner_class, new_class)
        return cls
    return inner_class_builder


# --------------------------------
#  Instrument and facility types
# --------------------------------
@sans_type("LOQ", "LARMOR", "SANS2D", "NoInstrument")
class SANSInstrument(object):
    pass


@sans_type("ISIS", "NoFacility")
class SANSFacility(object):
    pass


def convert_string_to_sans_instrument(to_convert):
    to_convert_cap = to_convert.upper()
    if to_convert_cap == SANSConstants.sans2d:
        selected_instrument = SANSInstrument.SANS2D
    elif to_convert_cap == SANSConstants.loq:
        selected_instrument = SANSInstrument.LOQ
    elif to_convert_cap == SANSConstants.larmor:
        selected_instrument = SANSInstrument.LARMOR
    else:
        selected_instrument = SANSInstrument.NoInstrument
    return selected_instrument


def convert_sans_instrument_to_string(to_convert):
    if to_convert is SANSInstrument.SANS2D:
        selected_instrument = SANSConstants.sans2d
    elif to_convert is SANSInstrument.LOQ:
        selected_instrument = SANSConstants.loq
    elif to_convert == SANSInstrument.LARMOR:
        selected_instrument = SANSConstants.larmor
    else:
        selected_instrument = ""
    return selected_instrument


# ------------------------------------
# Data Types
# ------------------------------------
@sans_type("SampleScatter", "SampleTransmission", "SampleDirect", "CanScatter", "CanTransmission", "CanDirect",
           "Calibration")
class SANSDataType(object):
    """
    Defines the different data types which are required for the reduction. Besides the fundamental data of the
    sample and the can, we can also specify a calibration.
    """
    pass


def convert_to_data_type(as_string):
    if as_string == "sample_scatter":
        data_type = SANSDataType.SampleScatter
    elif as_string == "sample_transmission":
        data_type = SANSDataType.SampleTransmission
    elif as_string == "sample_direct":
        data_type = SANSDataType.SampleDirect
    elif as_string == "can_scatter":
        data_type = SANSDataType.CanScatter
    elif as_string == "can_transmission":
        data_type = SANSDataType.CanTransmission
    elif as_string == "can_direct":
        data_type = SANSDataType.CanDirect
    elif as_string == "calibration":
        data_type = SANSDataType.Calibration
    else:
        raise ValueError("The specified data type {0} does not exist.".format(as_string))

    return data_type


def convert_from_data_type_to_string(data_type):
    if data_type is SANSDataType.SampleScatter:
        as_string = "sample_scatter"
    elif data_type is SANSDataType.SampleTransmission:
        as_string = "sample_transmission"
    elif data_type is SANSDataType.SampleDirect:
        as_string = "sample_direct"
    elif data_type is SANSDataType.CanScatter:
        as_string = "can_scatter"
    elif data_type is SANSDataType.CanTransmission:
        as_string = "can_transmission"
    elif data_type is SANSDataType.CanDirect:
        as_string = "can_direct"
    elif data_type is SANSDataType.Calibration:
        as_string = "calibration"
    else:
        raise ValueError("The specified data type {0} does not exist.".format(data_type))
    return as_string


# --------------------------
#  Coordinate Definitions (3D)
# --------------------------

class Coordinates(object):
    pass


@sans_type("X", "Y", "Z")
class CanonicalCoordinates(Coordinates):
    pass


# --------------------------
#  ReductionMode
# --------------------------
@sans_type("Merged", "All")
class ReductionMode(object):
    """
    Defines the reduction modes which should be common to all implementations, namely All and Merged.
    """
    pass


@sans_type("Hab", "Lab")
class ISISReductionMode(ReductionMode):
    """
    Defines the different reduction modes. This can be the high-angle bank, the low-angle bank
    """
    pass


# --------------------------
#  Reduction dimensionality
# --------------------------
@sans_type("OneDim", "TwoDim")
class ReductionDimensionality(object):
    """
    Defines the dimensionality for reduction. This can be either 1D or 2D
    """
    pass


# --------------------------
#  Reduction data
# --------------------------
@sans_type("Scatter", "Transmission", "Direct")
class ReductionData(object):
    """
    Defines the workspace type of the reduction data. For all known instances this can be scatter, transmission
    or direct
    """
    pass


# --------------------------
#  Type of data
# --------------------------
@sans_type("Sample", "Can")
class DataType(object):
    """
    Defines the type of reduction data. This can either the sample or only the can.
    """
    pass


def convert_reduction_data_type_to_string(data_type):
    if data_type is DataType.Sample:
        data_type_as_string = SANSConstants.sample
    elif data_type is DataType.Can:
        data_type_as_string = SANSConstants.can
    else:
        raise ValueError("DataType: The data type {0} is not known.".format(data_type))
    return data_type_as_string


def convert_string_to_reduction_data_type(data_string):
    if data_string == SANSConstants.sample:
        data_type = DataType.Sample
    elif data_string == SANSConstants.can:
        data_type = DataType.Can
    else:
        raise ValueError("DataType: The data string {0} is not known.".format(data_string))
    return data_type


# ---------------------------------
#  Partial reduction output setting
# ---------------------------------
@sans_type("Count", "Norm")
class OutputParts(object):
    """
    Defines the partial outputs of a reduction. They are the numerator (Count) and denominator (Norm) of a division.
    """
    pass


# -----------------------------------------------------
#  The fit type during merge of HAB and LAB reductions
# -----------------------------------------------------
@sans_type("Both", "NoFit", "ShiftOnly", "ScaleOnly")
class FitModeForMerge(object):
    """
    Defines which fit operation to use during the merge of two reductions.
    """
    pass


def convert_fit_mode_for_merge_to_string(to_convert):
    if to_convert is FitModeForMerge.Both:
        selected_fit_mode = "Both"
    elif to_convert is FitModeForMerge.NoFit:
        selected_fit_mode = "None"
    elif to_convert == FitModeForMerge.ShiftOnly:
        selected_fit_mode = "ShiftOnly"
    elif to_convert == FitModeForMerge.ScaleOnly:
        selected_fit_mode = "ScaleOnly"
    else:
        selected_fit_mode = ""
    return selected_fit_mode


# --------------------------
#  Detectors
# --------------------------
@sans_type("Horizontal", "Vertical", "Rotated")
class DetectorOrientation(object):
    """
    Defines the detector orientation.
    """
    pass


# --------------------------
#  Detector Type
# --------------------------
@sans_type("Hab", "Lab")
class DetectorType(object):
    """
    Defines the detector type
    """
    pass


def convert_detector_type_to_string(to_convert):
    if to_convert is DetectorType.Hab:
        detector_type_string = SANSConstants.high_angle_bank
    elif to_convert is DetectorType.Lab:
        detector_type_string = SANSConstants.low_angle_bank
    else:
        raise RuntimeError("Trying to convert a detector of type {0} to a string. Cannot handle this detector"
                           " type currently.".format(to_convert))
    return detector_type_string


def convert_string_to_detector_type(to_convert):
    if to_convert == SANSConstants.high_angle_bank:
        detector_type = DetectorType.Hab
    elif to_convert == SANSConstants.low_angle_bank:
        detector_type = DetectorType.Lab
    else:
        raise RuntimeError("Trying to convert a detector of type {0} to a string. Cannot handle this detector"
                           " type currently.".format(to_convert))
    return detector_type


# --------------------------
#  Ranges
# --------------------------
@sans_type("Lin", "Log")
class RangeStepType(object):
    """
    Defines the step type of a range
    """
    pass


def convert_range_step_type_to_string(range_type):
    if range_type is RangeStepType.Lin:
        range_string = SANSConstants.range_step_lin
    elif range_type is RangeStepType.Log:
        range_string = SANSConstants.range_step_log
    else:
        raise ValueError("RangeStepType: The range step type {0} is not known.".format(range_type))
    return range_string


def convert_string_to_range_step_type(range_string):
    if range_string == SANSConstants.range_step_lin:
        range_type = RangeStepType.Lin
    elif range_string == SANSConstants.range_step_log:
        range_type = RangeStepType.Log
    else:
        raise ValueError("RangeStepType: The range step string {0} is not known.".format(range_string))
    return range_type


# --------------------------
#  Rebin
# --------------------------
@sans_type("Rebin", "InterpolatingRebin")
class RebinType(object):
    """
    Defines the rebin types available
    """
    pass


def convert_rebin_type_to_string(rebin_type):
    if rebin_type is RebinType.Rebin:
        rebin_string = SANSConstants.rebin
    elif rebin_type is RebinType.InterpolatingRebin:
        rebin_string = SANSConstants.intperpolating_rebin
    else:
        raise ValueError("RebinType: The rebin type {0} is not known.".format(rebin_type))
    return rebin_string


def convert_string_to_rebin_type(rebin_string):
    if rebin_string == SANSConstants.rebin:
        rebin_type = RebinType.Rebin
    elif rebin_string == SANSConstants.intperpolating_rebin:
        rebin_type = RebinType.InterpolatingRebin
    else:
        raise ValueError("RebinType: The rebin string {0} is not known.".format(rebin_string))
    return rebin_type


# --------------------------
#  SaveType
# --------------------------
@sans_type("Nexus", "NistQxy", "CanSAS", "RKH", "CSV", "NXcanSAS")
class SaveType(object):
    """
    Defines the save types available
    """
    pass


def convert_save_type_to_string(save_type):
    if save_type is SaveType.Nexus:
        as_string = "Nexus"
    elif save_type is SaveType.NistQxy:
        as_string = "NistQxy"
    elif save_type is SaveType.CanSAS:
        as_string = "CanSAS"
    elif save_type is SaveType.RKH:
        as_string = "RKH"
    elif save_type is SaveType.CSV:
        as_string = "CSV"
    elif save_type is SaveType.NXcanSAS:
        as_string = "NXcanSAS"
    else:
        raise ValueError("SaveType: Cannot convert unknown save type: {0}".format(save_type))
    return as_string


# ------------------------------------------
# Fit type for the transmission calculation
# ------------------------------------------
@sans_type("Linear", "Log", "Polynomial", "NoFit")
class FitType(object):
    """
    Defines possible fit types
    """
    pass


def convert_fit_type_to_string(fit_type):
    as_string = None
    if fit_type is FitType.Linear:
        as_string = "Linear"
    elif fit_type is FitType.Log:
        as_string = "Log"
    elif fit_type is FitType.Polynomial:
        as_string = "Polynomial"
    elif fit_type is FitType.NoFit:
        as_string = "NoFit"
    return as_string


# --------------------------
#  SampleShape
# --------------------------
@sans_type("CylinderAxisUp", "Cuboid", "CylinderAxisAlong")
class SampleShape(object):
    """
    Defines the sample shape types
    """
    pass


def convert_int_to_shape(shape_int):
    if shape_int == 1:
        as_type = SampleShape.CylinderAxisUp
    elif shape_int == 2:
        as_type = SampleShape.Cuboid
    elif shape_int == 3:
        as_type = SampleShape.CylinderAxisAlong
    else:
        raise ValueError("SampleShape: Cannot convert unknown sample shape integer: {0}".format(shape_int))
    return as_type


# ---------------------------
# FileTypes
# ---------------------------
class SANSFileType(object):
    class ISISNexus(object):
        pass

    class ISISNexusAdded(object):
        pass

    class ISISRaw(object):
        pass

    class NoFileType(object):
        pass
