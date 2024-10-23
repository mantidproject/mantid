# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,unused-import
# ruff: noqa: F403   # Allow wild imports
"""
Command set for EQSANS reduction
"""

# Import the specific commands that we need - some of these are used in systemtests
from reduction_workflow.command_interface import *

# The following imports allow users to import this file and have all functionality automatically imported
# Do not remove these imports as it will break user scripts which rely on them

from .hfir_command_interface import DarkCurrent, NoDarkCurrent, NoNormalization  # noqa: F401
from .hfir_command_interface import SolidAngle, NoSolidAngle  # noqa: F401
from .hfir_command_interface import DirectBeamCenter, ScatteringBeamCenter  # noqa: F401
from .hfir_command_interface import SetBeamCenter as BaseSetBeamCenter

from .hfir_command_interface import SensitivityCorrection, SetSensitivityBeamCenter  # noqa: F401
from .hfir_command_interface import SensitivityDirectBeamCenter, SensitivityScatteringBeamCenter  # noqa: F401
from .hfir_command_interface import NoSensitivityCorrection, DivideByThickness  # noqa: F401

from .hfir_command_interface import IQxQy, NoIQxQy, SaveIq, NoSaveIq, SaveIqAscii  # noqa: F401

from .hfir_command_interface import DirectBeamTransmission, TransmissionDarkCurrent  # noqa: F401
from .hfir_command_interface import ThetaDependentTransmission  # noqa: F401
from .hfir_command_interface import SetTransmissionBeamCenter, TransmissionDirectBeamCenter  # noqa: F401
from .hfir_command_interface import SetTransmission, NoTransmission  # noqa: F401

from .hfir_command_interface import Background, NoBackground, NoBckTransmission  # noqa: F401
from .hfir_command_interface import SetBckTransmission, BckDirectBeamTransmission  # noqa: F401
from .hfir_command_interface import SetBckTransmissionBeamCenter, BckThetaDependentTransmission  # noqa: F401
from .hfir_command_interface import BckTransmissionDirectBeamCenter, BckTransmissionDarkCurrent  # noqa: F401

from .hfir_command_interface import SetSampleDetectorOffset, SetSampleDetectorDistance  # noqa: F401
from .hfir_command_interface import Mask, MaskRectangle, MaskDetectors, MaskDetectorSide  # noqa: F401
from .hfir_command_interface import SetAbsoluteScale, SetDirectBeamAbsoluteScale  # noqa: F401
from .hfir_command_interface import Stitch  # noqa: F401

from reduction_workflow.find_data import find_data


def EQSANS(keep_events=False, property_manager=None):
    Clear()
    ReductionSingleton().set_instrument("EQSANS", "SetupEQSANSReduction", "SANSReduction")
    ReductionSingleton().reduction_properties["PreserveEvents"] = keep_events
    SolidAngle()
    AzimuthalAverage()
    if property_manager is not None:
        ReductionSingleton().set_reduction_table_name(property_manager)


def SetBeamCenter(x, y):
    if x == 0 and y == 0:
        ReductionSingleton().reduction_properties["UseConfigBeam"] = True
    else:
        BaseSetBeamCenter(x, y)


def TotalChargeNormalization(normalize_to_beam=True, beam_file=""):
    if normalize_to_beam:
        ReductionSingleton().reduction_properties["Normalisation"] = "BeamProfileAndCharge"
        ReductionSingleton().reduction_properties["MonitorReferenceFile"] = beam_file
    else:
        ReductionSingleton().reduction_properties["Normalisation"] = "Charge"


def BeamMonitorNormalization(reference_flux_file):
    reference_flux_file = find_data(reference_flux_file, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["Normalisation"] = "Monitor"
    ReductionSingleton().reduction_properties["MonitorReferenceFile"] = reference_flux_file


def PerformFlightPathCorrection(do_correction=True):
    ReductionSingleton().reduction_properties["CorrectForFlightPath"] = do_correction


def SetTOFTailsCutoff(low_cut=0.0, high_cut=0.0):
    ReductionSingleton().reduction_properties["LowTOFCut"] = low_cut
    ReductionSingleton().reduction_properties["HighTOFCut"] = high_cut


def UseConfigTOFTailsCutoff(use_config=True):
    ReductionSingleton().reduction_properties["UseConfigTOFCuts"] = use_config


def SkipTOFCorrection(skip=True):
    ReductionSingleton().reduction_properties["SkipTOFCorrection"] = skip


def UseConfigMask(use_config=True):
    ReductionSingleton().reduction_properties["UseConfigMask"] = use_config


def SetWavelengthStep(step=0.1):
    ReductionSingleton().reduction_properties["WavelengthStep"] = step


def UseConfig(use_config=True):
    ReductionSingleton().reduction_properties["UseConfig"] = use_config


def AzimuthalAverage(suffix="_Iq", n_bins=100, n_subpix=1, log_binning=False, scale=True):
    # Suffix is no longer used but kept for backward compatibility
    # N_subpix is also no longer used
    ReductionSingleton().reduction_properties["DoAzimuthalAverage"] = True
    ReductionSingleton().reduction_properties["IQNumberOfBins"] = n_bins
    ReductionSingleton().reduction_properties["IQLogBinning"] = log_binning
    ReductionSingleton().reduction_properties["IQScaleResults"] = scale


def CombineTransmissionFits(combine_frames=True):
    ReductionSingleton().reduction_properties["FitFramesTogether"] = combine_frames


def BckCombineTransmissionFits(combine_frames=True):
    ReductionSingleton().reduction_properties["BckFitFramesTogether"] = combine_frames


def Resolution(sample_aperture_diameter=10.0):
    ReductionSingleton().reduction_properties["ComputeResolution"] = True
    ReductionSingleton().reduction_properties["SampleApertureDiameter"] = sample_aperture_diameter


def IndependentBinning(independent_binning=True):
    ReductionSingleton().reduction_properties["IQIndependentBinning"] = independent_binning


def SetDetectorOffset(distance):
    ReductionSingleton().reduction_properties["DetectorOffset"] = distance


def SetSampleOffset(distance):
    ReductionSingleton().reduction_properties["SampleOffset"] = distance


def LoadNexusInstrumentXML(value=True):
    ReductionSingleton().reduction_properties["LoadNexusInstrumentXML"] = value
