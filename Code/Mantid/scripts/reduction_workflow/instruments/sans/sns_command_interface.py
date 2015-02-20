#pylint: disable=invalid-name
"""
    Command set for EQSANS reduction
"""
# Import the specific commands that we need
from reduction_workflow.command_interface import *

#from hfir_command_interface import DarkCurrent, NoDarkCurrent, NoNormalization
from hfir_command_interface import SolidAngle#, NoSolidAngle
#from hfir_command_interface import DirectBeamCenter, ScatteringBeamCenter
from hfir_command_interface import SetBeamCenter as BaseSetBeamCenter

#from hfir_command_interface import SensitivityCorrection, SetSensitivityBeamCenter
#from hfir_command_interface import SensitivityDirectBeamCenter, SensitivityScatteringBeamCenter
#from hfir_command_interface import NoSensitivityCorrection, DivideByThickness

#from hfir_command_interface import IQxQy, NoIQxQy, SaveIq, NoSaveIq, SaveIqAscii

#from hfir_command_interface import DirectBeamTransmission, TransmissionDarkCurrent
#from hfir_command_interface import ThetaDependentTransmission
#from hfir_command_interface import SetTransmissionBeamCenter, TransmissionDirectBeamCenter
#from hfir_command_interface import SetTransmission, NoTransmission

#from hfir_command_interface import Background, NoBackground, NoBckTransmission
#from hfir_command_interface import SetBckTransmission, BckDirectBeamTransmission
#from hfir_command_interface import SetBckTransmissionBeamCenter, BckThetaDependentTransmission
#from hfir_command_interface import BckTransmissionDirectBeamCenter, BckTransmissionDarkCurrent

#from hfir_command_interface import SetSampleDetectorOffset, SetSampleDetectorDistance
#from hfir_command_interface import Mask, MaskRectangle, MaskDetectors, MaskDetectorSide
#from hfir_command_interface import SetAbsoluteScale, SetDirectBeamAbsoluteScale
#from hfir_command_interface import Stitch

#from mantid.api import AlgorithmManager
#from mantid.kernel import Logger
#import mantid.simpleapi as simpleapi
from reduction_workflow.find_data import find_data

def EQSANS(keep_events=False, property_manager=None):
    Clear()
    ReductionSingleton().set_instrument("EQSANS",
                                        "SetupEQSANSReduction",
                                        "SANSReduction")
    ReductionSingleton().reduction_properties["PreserveEvents"]=keep_events
    SolidAngle()
    AzimuthalAverage()
    if property_manager is not None:
        ReductionSingleton().set_reduction_table_name(property_manager)

def SetBeamCenter(x,y):
    if x==0 and y==0:
        ReductionSingleton().reduction_properties["UseConfigBeam"]=True
    else:
        BaseSetBeamCenter(x,y)

def TotalChargeNormalization(normalize_to_beam=True, beam_file=''):
    if normalize_to_beam:
        ReductionSingleton().reduction_properties["Normalisation"]="BeamProfileAndCharge"
        ReductionSingleton().reduction_properties["MonitorReferenceFile"]=beam_file
    else:
        ReductionSingleton().reduction_properties["Normalisation"]="Charge"

def BeamMonitorNormalization(reference_flux_file):
    reference_flux_file = find_data(reference_flux_file, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["Normalisation"]="Monitor"
    ReductionSingleton().reduction_properties["MonitorReferenceFile"]=reference_flux_file

def PerformFlightPathCorrection(do_correction=True):
    ReductionSingleton().reduction_properties["CorrectForFlightPath"]=do_correction

def SetTOFTailsCutoff(low_cut=0.0, high_cut=0.0):
    ReductionSingleton().reduction_properties["LowTOFCut"]=low_cut
    ReductionSingleton().reduction_properties["HighTOFCut"]=high_cut

def UseConfigTOFTailsCutoff(use_config=True):
    ReductionSingleton().reduction_properties["UseConfigTOFCuts"]=use_config

def SkipTOFCorrection(skip=True):
    ReductionSingleton().reduction_properties["SkipTOFCorrection"]=skip

def UseConfigMask(use_config=True):
    ReductionSingleton().reduction_properties["UseConfigMask"]=use_config

def SetWavelengthStep(step=0.1):
    ReductionSingleton().reduction_properties["WavelengthStep"]=step

def UseConfig(use_config=True):
    ReductionSingleton().reduction_properties["UseConfig"]=use_config

def AzimuthalAverage(suffix="_Iq", n_bins=100, n_subpix=1, log_binning=False,
                     scale=True):
    # Suffix is no longer used but kept for backward compatibility
    # N_subpix is also no longer used
    ReductionSingleton().reduction_properties["DoAzimuthalAverage"]=True
    ReductionSingleton().reduction_properties["IQNumberOfBins"]=n_bins
    ReductionSingleton().reduction_properties["IQLogBinning"]=log_binning
    ReductionSingleton().reduction_properties["IQScaleResults"]=scale

def CombineTransmissionFits(combine_frames=True):
    ReductionSingleton().reduction_properties["FitFramesTogether"]=combine_frames

def BckCombineTransmissionFits(combine_frames=True):
    ReductionSingleton().reduction_properties["BckFitFramesTogether"]=combine_frames

def Resolution(sample_aperture_diameter=10.0):
    ReductionSingleton().reduction_properties["ComputeResolution"]=True
    ReductionSingleton().reduction_properties["SampleApertureDiameter"]=sample_aperture_diameter

def IndependentBinning(independent_binning=True):
    ReductionSingleton().reduction_properties["IQIndependentBinning"]=independent_binning
