# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The elements of this module define typed enums which are used in the SANS reduction framework."""

# pylint: disable=too-few-public-methods, invalid-name

from __future__ import (absolute_import, division, print_function)
from inspect import isclass
from functools import partial
from six import PY2


# ----------------------------------------------------------------------------------------------------------------------
# Serializable Enum decorator
# ----------------------------------------------------------------------------------------------------------------------
from mantid.py3compat import Enum


def serializable_enum(*inner_classes):
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


# ----------------------------------------------------------------------------------------------------------------------
# String conversion decorator
# ----------------------------------------------------------------------------------------------------------------------
def string_convertible(cls):
    """
    Class decorator to make the enum/sub-class entries string convertible.

    We do this by creating a static  from_string and to_string method on the class.
    IMPORTANT: It is important that the enum values are added to the class before applying this decorator. In general
               the order has to be:
               @string_convertible
               @serializable_enum
               class MyClass(object):
                ...
    :param cls: a reference to the class
    :return: the class
    """
    def to_string(elements, convert_to_string):
        for key, value in list(elements.items()):
            if convert_to_string is value:
                return key
        raise RuntimeError("Could not convert {0} to string. Unknown value.".format(convert_to_string))

    def from_string(elements, convert_from_string):
        if not PY2 and isinstance(convert_from_string, bytes):
            convert_from_string = convert_from_string.decode()
        for key, value in list(elements.items()):
            if convert_from_string == key:
                return value
        raise RuntimeError("Could not convert {0} from string. Unknown value.".format(convert_from_string))

    def has_member(elements, convert):
        if not PY2 and isinstance(convert, bytes):
            convert = convert.decode()
        for key, value in list(elements.items()):
            if convert == key or convert == value:
                return True
        return False

    # First get all enum/sub-class elements
    convertible_elements = {}
    for attribute_name, attribute_value in list(cls.__dict__.items()):
        if isclass(attribute_value) and issubclass(attribute_value, cls):
            convertible_elements.update({attribute_name: attribute_value})

    # Add the new static methods to the class
    partial_to_string = partial(to_string, convertible_elements)
    partial_from_string = partial(from_string, convertible_elements)
    partial_has_member = partial(has_member, convertible_elements)
    setattr(cls, "to_string", staticmethod(partial_to_string))
    setattr(cls, "from_string", staticmethod(partial_from_string))
    setattr(cls, "has_member", staticmethod(partial_has_member))
    return cls


class SANSInstrument(Enum):
    NO_INSTRUMENT = "No Instrument"

    LARMOR = "LARMOR"
    LOQ = "LOQ"
    SANS2D = "SANS2D"
    ZOOM = "ZOOM"


class SANSFacility(Enum):
    NO_FACILITY = "No Facility"
    ISIS = "ISIS"


class SANSDataType(Enum):
    """
    Defines the different data types which are required for the reduction. Besides the fundamental data of the
    sample and the can, we can also specify a calibration.
    """
    CAN_DIRECT = "Can Direct"
    CAN_TRANSMISSION = "Can Transmission"
    CAN_SCATTER = "Can Scatter"
    CALIBRATION = "Calibration"
    SAMPLE_DIRECT = "Sample Direct"
    SAMPLE_SCATTER = "Sample Scatter"
    SAMPLE_TRANSMISSION = "Sample Transmission"


class CanonicalCoordinates(Enum):
    X = "X"
    Y = "Y"
    Z = "Z"


class ReductionMode(Enum):
    NOT_SET = "Not Set"
    ALL = "All"
    MERGED = "Merged"
    HAB = "HAB"
    LAB = "LAB"


class ReductionDimensionality(Enum):
    ONE_DIM = "OneDim"
    TWO_DIM = "TwoDim"


class ReductionData(Enum):
    """
    Defines the workspace type of the reduction data. For all known instances this can be scatter, transmission
    or direct
    """
    DIRECT = "Direct"
    SCATTER = "Scatter"
    TRANSMISSION = "Transmission"


class DataType(Enum):
    """
    Defines the type of reduction data. This can either the sample or only the can.
    """
    CAN = "Can"
    SAMPLE = "Sample"


class OutputParts(Enum):
    """
    Defines the partial outputs of a reduction. They are the numerator (Count) and denominator (Norm) of a division.
    """
    COUNT = "Count"
    NORM = "Norm"


class FitModeForMerge(Enum):
    """
    Defines which fit operation to use during the merge of two reductions.
    """
    BOTH = "Both"
    NO_FIT = "NoFit"
    SCALE_ONLY = "ScaleOnly"
    SHIFT_ONLY = "ShiftOnly"


class DetectorType(Enum):
    """
    Defines the detector type
    """
    HAB = "HAB"
    LAB = "LAB"


class TransmissionType(Enum):
    """
    Defines the detector type
    """
    CALCULATED = "Calculated"
    UNFITTED = "Unfitted"


class RangeStepType(Enum):
    """
    Defines the step type of a range
    """
    LIN = "Lin"
    LOG = "Log"
    NOT_SET = "NotSet"
    RANGE_LIN = "RangeLin"
    RANGE_LOG = "RangeLog"

# --------------------------
#  Rebin
# --------------------------
@string_convertible
@serializable_enum("Rebin", "InterpolatingRebin")
class RebinType(object):
    """
    Defines the rebin types available
    """
    pass


# --------------------------
#  SaveType
# --------------------------
@string_convertible
@serializable_enum("Nexus", "NistQxy", "CanSAS", "RKH", "CSV", "NXcanSAS", "Nexus", "NoType")
class SaveType(object):
    """
    Defines the save types available
    """
    pass


# ------------------------------------------
# Fit type for the transmission calculation
# ------------------------------------------
@string_convertible
@serializable_enum("Linear", "Logarithmic", "Polynomial", "NoFit")
class FitType(object):
    """
    Defines possible fit types
    """
    pass


# --------------------------
#  SampleShape
# --------------------------
@string_convertible
@serializable_enum("Cylinder", "FlatPlate", "Disc")
class SampleShape(object):
    """
    Defines the sample shape types
    """
    pass


def convert_int_to_shape(shape_int):
    """
    Note that we convert the sample shape to an integer here. This is required for the workspace, hence we don't
    use the string_convertible decorator.
    """
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
@serializable_enum("ISISNexus", "ISISNexusAdded", "ISISRaw", "NoFileType")
class FileType(object):
    pass


# ---------------------------
# OutputMode
# ---------------------------
@string_convertible
@serializable_enum("PublishToADS", "SaveToFile", "Both")
class OutputMode(object):
    """
    Defines the output modes of a batch reduction.
    """
    pass


# ------------------------------
# Entries of batch reduction file
# -------------------------------
@string_convertible
@serializable_enum("SampleScatter", "SampleTransmission", "SampleDirect", "CanScatter", "CanTransmission", "CanDirect",
                   "Output", "UserFile", "SampleScatterPeriod", "SampleTransmissionPeriod", "SampleDirectPeriod",
                   "CanScatterPeriod", "CanTransmissionPeriod", "CanDirectPeriod", "SampleThickness", "SampleHeight",
                   "SampleWidth")
class BatchReductionEntry(object):
    """
    Defines the entries of a batch reduction file.
    """
    pass


# ------------------------------
# Quadrants for beam centre finder
# -------------------------------
@string_convertible
@serializable_enum("Left", "Right", "Top", "Bottom")
class MaskingQuadrant(object):
    """
    Defines the entries of a batch reduction file.
    """
    pass


# ------------------------------
# Directions for Beam centre finder
# -------------------------------
@string_convertible
@serializable_enum("All", "Up_Down", "Left_Right")
class FindDirectionEnum(object):
    """
    Defines the entries of a batch reduction file.
    """
    pass


# ------------------------------
# Integrals for diagnostic tab
# -------------------------------
@string_convertible
@serializable_enum("Horizontal", "Vertical", "Time")
class IntegralEnum(object):
    """
    Defines the entries of a batch reduction file.
    """
    pass


@string_convertible
@serializable_enum("Unprocessed", "Processed", "Error")
class RowState(object):
    """
    Defines the entries of a batch reduction file.
    """
    pass


# ------------------------------
# Binning Types for AddRuns
# -------------------------------
@string_convertible
@serializable_enum("SaveAsEventData", "Custom", "FromMonitors")
class BinningType(object):
    """
    Defines the types of binning when adding runs together
    """
