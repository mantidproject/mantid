# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The elements of this module define typed enums which are used in the SANS reduction framework."""

from enum import Enum
from SANS.sans.state.JsonSerializable import json_serializable


@json_serializable
class SANSInstrument(Enum):
    NO_INSTRUMENT = "No Instrument"

    LARMOR = "LARMOR"
    LOQ = "LOQ"
    SANS2D = "SANS2D"
    ZOOM = "ZOOM"


@json_serializable
class SANSDetector(Enum):
    SANS2D_HAB = "front-detector"
    SANS2D_LAB = "rear-detector"
    LOQ_HAB = "HAB"
    LOQ_LAB = "main-detector-bank"
    LARMOR_LAB = "DetectorBench"
    ZOOM_LAB = "rear-detector"


@json_serializable
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


@json_serializable
class CanonicalCoordinates(Enum):
    X = "X"
    Y = "Y"
    Z = "Z"


@json_serializable
class CorrectionType(Enum):
    X = "X"
    Y = "Y"
    Z = "Z"

    X_TILT = "X_TILT"
    Y_TILT = "Y_TILT"

    RADIUS = "RADIUS"
    ROTATION = "ROTATION"
    SIDE = "SIDE"
    TRANSLATION = "TRANSLATION"


@json_serializable
class ReductionMode(Enum):
    NOT_SET = "Not Set"
    ALL = "All"
    MERGED = "Merged"
    HAB = "HAB"
    LAB = "LAB"

    @staticmethod
    def convert(val: str, *, support_deprecated=True):
        """
        Converts the existing legacy format enum values into the new front/rear types
        :param val: String value to convert
        :param support_deprecated: (Optional) support HAB/LAB legacy input. True by default, throws when False.
        :return: Reduction Mode enum, or throws if unsupported
        """
        mapping = {
            "HAB": ReductionMode.HAB,
            "front": ReductionMode.HAB,
            "LAB": ReductionMode.LAB,
            "rear": ReductionMode.LAB,
            "All": ReductionMode.ALL,
            "Merged": ReductionMode.MERGED,
        }
        if not support_deprecated and next((key for key in ["hab", "lab"] if val.casefold() == key.casefold()), None):
            raise ValueError(f"A deprecated key was found: {val}.\nPlease use front/rear as appropriate instead.")

        # Case insensitive key search
        enum_val = next((enum_val for key, enum_val in mapping.items() if val.casefold() == key.casefold()), None)
        if not enum_val:
            ReductionMode(val)  # Rethrow using enums built in exception
        return enum_val


@json_serializable
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


@json_serializable
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

    BOTH = "BOTH"
    HAB = "HAB"
    LAB = "LAB"


class TransmissionType(Enum):
    """
    Defines the detector type
    """

    CALCULATED = "Calculated"
    UNFITTED = "Unfitted"


@json_serializable
class RangeStepType(Enum):
    """
    Defines the step type of a range
    """

    LIN = "Lin"
    LOG = "Log"
    NOT_SET = "NotSet"
    RANGE_LIN = "RangeLin"
    RANGE_LOG = "RangeLog"


@json_serializable
class RebinType(Enum):
    INTERPOLATING_REBIN = "InterpolatingRebin"
    REBIN = "Rebin"


@json_serializable
class SaveType(Enum):
    CAN_SAS = "CanSAS"
    CSV = "CSV"
    NEXUS = "Nexus"
    NIST_QXY = "NistQxy"
    NO_TYPE = "NoType"
    NX_CAN_SAS = "NXcanSAS"
    RKH = "RKH"


@json_serializable
class FitType(Enum):
    """
    Defines possible fit types for the transmission calculation
    """

    LINEAR = "Linear"
    LOGARITHMIC = "Logarithmic"
    POLYNOMIAL = "Polynomial"
    NO_FIT = "NoFit"


@json_serializable
class SampleShape(Enum):
    """
    Defines the sample shape types
    """

    CYLINDER = "Cylinder"
    DISC = "Disc"
    FLAT_PLATE = "FlatPlate"
    NOT_SET = "NotSet"


class FileType(Enum):
    ISIS_NEXUS = "ISISNexus"
    ISIS_NEXUS_ADDED = "ISISNexusAdded"
    ISIS_RAW = "ISISRaw"
    NO_FILE_TYPE = "NoFileType"


class OutputMode(Enum):
    """
    Defines the output modes of a batch reduction.
    """

    BOTH = "Both"
    PUBLISH_TO_ADS = "PublishToADS"
    SAVE_TO_FILE = "SaveToFile"


class BatchReductionEntry(Enum):
    """
    Defines the entries of a batch reduction file.
    """

    CAN_DIRECT = "CanDirect"
    CAN_DIRECT_PERIOD = "CanDirectPeriod"

    CAN_SCATTER = "CanScatter"
    CAN_SCATTER_PERIOD = "CanScatterPeriod"

    CAN_TRANSMISSION = "CanTransmission"
    CAN_TRANSMISSION_PERIOD = "CanTransmissionPeriod"

    OUTPUT = "Output"

    SAMPLE_DIRECT = "SampleDirect"
    SAMPLE_DIRECT_PERIOD = "SampleDirectPeriod"

    SAMPLE_SCATTER = "SampleScatter"
    SAMPLE_SCATTER_PERIOD = "SampleScatterPeriod"

    SAMPLE_TRANSMISSION = "SampleTransmission"
    SAMPLE_TRANSMISSION_PERIOD = "SampleTransmissionPeriod"

    SAMPLE_HEIGHT = "SampleHeight"
    SAMPLE_THICKNESS = "SampleThickness"
    SAMPLE_WIDTH = "SampleWidth"

    USER_FILE = "UserFile"


class MaskingQuadrant(Enum):
    """
    Defines the entries of a batch reduction file.
    """

    BOTTOM = "Bottom"
    LEFT = "Left"
    RIGHT = "Right"
    TOP = "Top"


class FindDirectionEnum(Enum):
    """
    Defines the entries of a batch reduction file.
    """

    ALL = "All"
    UP_DOWN = "Up_Down"
    LEFT_RIGHT = "Left_Right"


class IntegralEnum(Enum):
    """
    Defines the entries of a batch reduction file.
    """

    Horizontal = "Horizontal"
    Time = "Time"
    Vertical = "Vertical"


class RowState(Enum):
    """
    Defines the entries of a batch reduction file.
    """

    ERROR = "Error"
    PROCESSED = "Processed"
    UNPROCESSED = "Unprocessed"


class BinningType(Enum):
    """
    Defines the types of binning when adding runs together
    """

    CUSTOM = "Custom"
    FROM_MONITORS = "FromMonitors"
    SAVE_AS_EVENT_DATA = "SaveAsEventData"
