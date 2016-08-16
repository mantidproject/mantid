# pylint: disable=too-few-public-methods, invalid-name

from SANS2.Common.SANSConstants import SANSConstants


# -------------------------------------------
# Strongly Typed enum class decorator
# -------------------------------------------
def inner_classes_with_name_space(*inner_classes):
    """
    Class decorator which changes the name of an inner class to include the name of the outer class. The inner class
    gets a method to determine the name of the outer class. This information is needed for serialization at the
    algorithm input boundary
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


# ---------------------------
#  Instrument and facility types
# --------------------------
@inner_classes_with_name_space("LOQ", "LARMOR", "SANS2D", "NoInstrument")
class SANSInstrument(object):
    pass


@inner_classes_with_name_space("ISIS", "NoFacility")
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
@inner_classes_with_name_space("SampleScatter", "SampleTransmission", "SampleDirect",
                               "CanScatter", "CanTransmission", "CanDirect",
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
    return as_string


# --------------------------
#  Coordinate Definitions (3D)
# --------------------------

class Coordinates(object):
    pass


@inner_classes_with_name_space("X", "Y", "Z")
class CanonicalCoordinates(Coordinates):
    pass


# --------------------------
#  ReductionMode
# --------------------------
@inner_classes_with_name_space("Merged", "All")
class ReductionMode(object):
    """
    Defines the reduction modes which should be common to all implementations, namely All and Merged.
    """
    pass


@inner_classes_with_name_space("Hab", "Lab")
class ISISReductionMode(ReductionMode):
    """
    Defines the different reduction modes. This can be the high-angle bank, the low-angle bank
    """
    pass


@inner_classes_with_name_space("OneDim", "TwoDim")
class ReductionDimensionality(object):
    """
    Defines the dimensionality for reduction. This can be either 1D or 2D
    """
    pass


@inner_classes_with_name_space("Scatter", "Transmission", "Direct")
class ReductionData(object):
    """
    Defines the workspace type of the reduction data. For all known instances this can be scatter, transmission
    or direct
    """
    pass


@inner_classes_with_name_space("Sample", "Can")
class DataType(object):
    """
    Defines the type of reduction data. This can be either be with the sample or without the sample, i.e. Can
    """
    pass


@inner_classes_with_name_space("Count", "Norm")
class OutputParts(object):
    """
    Defines the partial outputs of a reduction. They are the numerator (Count) and denominator (Norm) of a division.
    """
    pass


@inner_classes_with_name_space("Both", "None", "ShiftOnly", "ScaleOnly")
class FitModeForMerge(object):
    """
    Defines which fit operation to use during the merge of two reductions.
    """
    pass


def convert_fit_mode_for_merge_to_string(to_convert):
    if to_convert is FitModeForMerge.Both:
        selected_fit_mode = "Both"
    elif to_convert is FitModeForMerge.None:
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
@inner_classes_with_name_space("Horizontal", "Vertical", "Rotated")
class DetectorOrientation(object):
    """
    Defines the detector orientation.
    """
    pass


@inner_classes_with_name_space("Hab", "Lab")
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


# --------------------------
#  Ranges
# --------------------------
@inner_classes_with_name_space("Lin", "Log")
class RangeStepType(object):
    """
    Defines the step type of a range
    """
    pass


# --------------------------
#  Rebin
# --------------------------
@inner_classes_with_name_space("Rebin", "InterpolatingRebin")
class RebinType(object):
    """
    Defines the rebin types available
    """
    pass


# --------------------------
#  SaveType
# --------------------------
@inner_classes_with_name_space("Nexus", "NistQxy", "CanSAS", "RKH", "CSV", "NXcanSAS")
class SaveType(object):
    """
    Defines the save types available
    """
    pass
