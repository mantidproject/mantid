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
    "SANS_ENUMS"
]


# --------------------------------
#  Instrument and facility types
# --------------------------------
SANSInstrument = Enum("SANSInstrument", "LOQ LARMOR SANS2D ZOOM NoInstrument")
SANSFacility = Enum("SANSFacility", "ISIS NoFacility")


# ------------------------------------
# Data Types
# ------------------------------------

# Defines the different data types which are required for the reduction. Besides the fundamental data of the sample
# and the can, we can also specify a calibration
SANSDataType = Enum("SANSDataType", "SampleScatter SampleTransmission SampleDirect "
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
ReductionMode = Enum("ReductionMode", "Merged All")


# Defines the different reduction modes. This can be the high-angle bank, the low-angle bank
class ISISReductionMode(Enum):
    Merged = 1
    All = 2
    HAB = 3
    LAB = 4

    def __eq__(self, other):
        if other is ReductionMode.Merged:
            return True if self.name == "Merged" else False
        elif other is ReductionMode.All:
            return True if self.name == "All" else False
        return super(ISISReductionMode, self).__eq__(other)


# --------------------------
#  Reduction dimensionality
# --------------------------
# Defines the dimensionality for reduction. This can be either 1D or 2D
ReductionDimensionality = Enum("ReductionDimensionality", "OneDim TwoDim")


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
DataType = Enum("DataType", "Sample Can")


# ---------------------------------
#  Partial reduction output setting
# ---------------------------------
# Defines the partial outputs of a reduction. They are the numerator (Count) and denominator (Norm) of a division
OutputParts = Enum("OutputParts", "Count Norm")


# -----------------------------------------------------
#  The fit type during merge of HAB and LAB reductions
# -----------------------------------------------------
# Defines which fit operation to use during the merge of two reductions
FitModeForMerge = Enum("FitModeForMerge", "Both NoFit ShiftOnly ScaleOnly")


# --------------------------
#  Detectors
# --------------------------
DetectorOrientation = Enum("DetectorOrientation", "Horizontal Vertical Rotated")


# --------------------------
#  Detector Type
# --------------------------
DetectorType = Enum("DetectorType", "HAB LAB")


# --------------------------
#  Transmission Type
# --------------------------
TransmissionType = Enum("TransmissionType", "Calculated Unfitted")


# --------------------------
#  Ranges
# --------------------------
# Defines the step type of a range
RangeStepType = Enum("RangeStepType", "Lin Log RangeLin RangeLog")


# --------------------------
#  Rebin
# --------------------------
RebinType = Enum("RebinType", "Rebin InterpolatingRebin")


# --------------------------
#  SaveType
# --------------------------
SaveType = Enum("SaveType", "Nexus NistQxy CanSAS RKH CSV NXcanSAS NoType")


# ------------------------------------------
# Fit type for the transmission calculation
# ------------------------------------------
FitType = Enum("FitType", "Linear Logarithmic Polynomial NoFit")


# --------------------------
#  SampleShape
# --------------------------
SampleShape = Enum("SampleShape", "Cylinder FlatPlate Disc")


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
OutputMode = Enum("OutputMode", "PublishToADS SaveToFile Both")


# ------------------------------
# Entries of batch reduction file
# -------------------------------
BatchReductionEntry = Enum("BatchReductionEntry", "SampleScatter SampleTransmission SampleDirect CanScatter "
                                                  "CanTransmission CanDirect Output UserFile SampleScatterPeriod "
                                                  "SampleTransmissionPeriod SampleDirectPeriod CanScatterPeriod "
                                                  "CanTransmissionPeriod CanDirectPeriod")


# ------------------------------
# Quadrants for beam centre finder
# -------------------------------
MaskingQuadrant = Enum("MaskingQuadrant", "Left Right Top Bottom")


# ------------------------------
# Directions for Beam centre finder
# -------------------------------
FindDirectionEnum = Enum("FindDirectionEnum", "All Up_Down Left_Right")


# ------------------------------
# Integrals for diagnostic tab
# -------------------------------
IntegralEnum = Enum("IntegralEnum", "Horizontal Vertical Time")
RowState = Enum("RowState", "Unprocessed Processed Error")


# ------------------------------
# Binning Types for AddRuns
# -------------------------------
BinningType = Enum("BinningType", "SaveAsEventData Custom FromMonitors")


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
