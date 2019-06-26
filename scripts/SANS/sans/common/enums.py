# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The elements of this module define typed enums which are used in the SANS reduction framework."""

# pylint: disable=too-few-public-methods, invalid-name

from __future__ import (absolute_import, division, print_function)

from mantid.py3compat import Enum


__all__ = [
    "SANSInstrument",
    "SANSFacility",
    "SANSDataType",
    "CanonicalCoordinates",
    "ReductionMode",
    "ISISReductionMode",
    "ReductionDimensionality",
    "ReductionData",
    "DataType",
    "OutputParts",
    "FitModeForMerge",
    "DetectorOrientation",
    "DetectorType",
    "TransmissionType",
    "RangeStepType",
    "RebinType",
    "SaveType",
    "FitType",
    "SampleShape",
    "FileType",
    "OutputMode",
    "BatchReductionEntry",
    "MaskingQuadrant",
    "FindDirectionEnum",
    "IntegralEnum",
    "RowState",
    "BinningType",
    "SANSEnum",
    "SANS_ENUMS"
]


class SANSEnum(Enum):
    """
    This class extends Enum by adding a method to check if the enum
    contains a certain member. This method is required for compatibility
    with the old SANS enum setup.
    """
    @classmethod
    def has_member(cls, member):
        """
        Check if member is part of the enum class
        :param member: the thing to check if it's part of the class
        :type member: str or Enum
        :return: True if member in cls, else False
        """
        for string_value, enum_value in cls._member_map_.items():
            print(string_value, enum_value)
            if member == string_value or member == enum_value:
                return True
        return False


# --------------------------------
#  Instrument and facility types
# --------------------------------
SANSInstrument = SANSEnum("SANSInstrument", "LOQ LARMOR SANS2D ZOOM NoInstrument")
SANSFacility = SANSEnum("SANSFacility", "ISIS NoFacility")


# ------------------------------------
# Data Types
# ------------------------------------

# Defines the different data types which are required for the reduction. Besides the fundamental data of the sample
# and the can, we can also specify a calibration
SANSDataType = SANSEnum("SANSDataType", "SampleScatter SampleTransmission SampleDirect "
                                        "CanScatter CanTransmission CanDirect Calibration")


# ---------------------------
#  Coordinate Definitions (3D)
# --------------------------
class Coordinates(object):
    pass


CanonicalCoordinates = Enum("CanonicalCoordinates", "X Y Z")


# --------------------------
#  ReductionMode
# --------------------------
# Defines the reduction modes which should be common to all implementations, namely All and Merged
ReductionMode = SANSEnum("ReductionMode", "Merged All")

# Defines the different reduction modes. This can be the high-angle bank, the low-angle bank
ISISReductionMode = SANSEnum("ISISReductionMode", "Merged All HAB LAB")


# --------------------------
#  Reduction dimensionality
# --------------------------
# Defines the dimensionality for reduction. This can be either 1D or 2D
ReductionDimensionality = SANSEnum("ReductionDimensionality", "OneDim TwoDim")


# --------------------------
#  Reduction data
# --------------------------
# Defines the workspace type of the reduction data. For all known instances this can be scatter, transmission
# or direct
ReductionData = Enum("ReductionData", "Scatter Transmission Direct")


# --------------------------
#  Type of data
# --------------------------
# Defines the type of reduction data. This can either the sample or only the can
DataType = SANSEnum("DataType", "Sample Can")


# ---------------------------------
#  Partial reduction output setting
# ---------------------------------
# Defines the partial outputs of a reduction. They are the numerator (Count) and denominator (Norm) of a division
OutputParts = Enum("OutputParts", "Count Norm")


# -----------------------------------------------------
#  The fit type during merge of HAB and LAB reductions
# -----------------------------------------------------
# Defines which fit operation to use during the merge of two reductions
FitModeForMerge = SANSEnum("FitModeForMerge", "Both NoFit ShiftOnly ScaleOnly")


# --------------------------
#  Detectors
# --------------------------
DetectorOrientation = Enum("DetectorOrientation", "Horizontal Vertical Rotated")


# --------------------------
#  Detector Type
# --------------------------
DetectorType = SANSEnum("DetectorType", "HAB LAB")


# --------------------------
#  Transmission Type
# --------------------------
TransmissionType = SANSEnum("TransmissionType", "Calculated Unfitted")


# --------------------------
#  Ranges
# --------------------------
# Defines the step type of a range
RangeStepType = SANSEnum("RangeStepType", "Lin Log RangeLin RangeLog")


# --------------------------
#  Rebin
# --------------------------
RebinType = SANSEnum("RebinType", "Rebin InterpolatingRebin")


# --------------------------
#  SaveType
# --------------------------
SaveType = SANSEnum("SaveType", "Nexus NistQxy CanSAS RKH CSV NXcanSAS NoType")


# ------------------------------------------
# Fit type for the transmission calculation
# ------------------------------------------
FitType = SANSEnum("FitType", "Linear Logarithmic Polynomial NoFit")


# --------------------------
#  SampleShape
# --------------------------
SampleShape = SANSEnum("SampleShape", "Cylinder FlatPlate Disc")


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
FileType = Enum("FileType", "ISISNexus ISISNexusAdded ISISRaw NoFileType")


# ---------------------------
# OutputMode
# ---------------------------
OutputMode = SANSEnum("OutputMode", "PublishToADS SaveToFile Both")


# ------------------------------
# Entries of batch reduction file
# -------------------------------
BatchReductionEntry = SANSEnum("BatchReductionEntry", "SampleScatter SampleTransmission SampleDirect CanScatter "
                                                      "CanTransmission CanDirect Output UserFile SampleScatterPeriod "
                                                      "SampleTransmissionPeriod SampleDirectPeriod CanScatterPeriod "
                                                      "CanTransmissionPeriod CanDirectPeriod")


# ------------------------------
# Quadrants for beam centre finder
# -------------------------------
MaskingQuadrant = SANSEnum("MaskingQuadrant", "Left Right Top Bottom")


# ------------------------------
# Directions for Beam centre finder
# -------------------------------
FindDirectionEnum = SANSEnum("FindDirectionEnum", "All Up_Down Left_Right")


# ------------------------------
# Integrals for diagnostic tab
# -------------------------------
IntegralEnum = SANSEnum("IntegralEnum", "Horizontal Vertical Time")
RowState = SANSEnum("RowState", "Unprocessed Processed Error")


# ------------------------------
# Binning Types for AddRuns
# -------------------------------
BinningType = SANSEnum("BinningType", "SaveAsEventData Custom FromMonitors")


# -------------------------------
# Dict of Enums for JSON encoding
# -------------------------------
SANS_ENUMS = {
    "SANSInstrument": SANSInstrument,
    "SANSFacility": SANSFacility,
    "SANSDataType": SANSDataType,
    "CanonicalCoordinates": CanonicalCoordinates,
    "ReductionMode": ReductionMode,
    "ISISReductionMode": ISISReductionMode,
    "ReductionDimensionality": ReductionDimensionality,
    "ReductionData": ReductionData,
    "DataType": DataType,
    "OutputParts": OutputParts,
    "FitModeForMerge": FitModeForMerge,
    "DetectorOrientation": DetectorOrientation,
    "DetectorType": DetectorType,
    "TransmissionType": TransmissionType,
    "RangeStepType": RangeStepType,
    "RebinType": RebinType,
    "SaveType": SaveType,
    "FitType": FitType,
    "SampleShape": SampleShape,
    "FileType": FileType,
    "OutputMode": OutputMode,
    "BatchReductionEntry": BatchReductionEntry,
    "MaskingQuadrant": MaskingQuadrant,
    "FindDirectionEnum": FindDirectionEnum,
    "IntegralEnum": IntegralEnum,
    "RowState": RowState,
    "BinningType": BinningType
}
