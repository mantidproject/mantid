"""
    Command set for EQSANS reduction
"""
# Import the specific commands that we need
from reduction.command_interface import *
from hfir_command_interface import SetBeamCenter, DirectBeamCenter, ScatteringBeamCenter
from hfir_command_interface import NoDarkCurrent, NoNormalization, Mask, MaskDetectors, MaskRectangle
from hfir_command_interface import SensitivityCorrection, NoSensitivityCorrection
from hfir_command_interface import SolidAngle, NoSolidAngle, NoTransmission, SetTransmission
from hfir_command_interface import Background, NoBackground, AzimuthalAverage, IQxQy, NoIQxQy
from hfir_command_interface import SaveIqAscii, NoSaveIq
from hfir_command_interface import ThetaDependentTransmission, BckThetaDependentTransmission, SetBckTransmission
from hfir_command_interface import TransmissionDarkCurrent, BckTransmissionDarkCurrent
from hfir_command_interface import SetDirectBeamAbsoluteScale, SetAbsoluteScale
from sns_reducer import EqSansReducer
import sns_instrument
import sns_reduction_steps

def EQSANS():
    Clear(EqSansReducer)
    ReductionSingleton().set_instrument(sns_instrument.EQSANS())
    NoSolidAngle()
    AzimuthalAverage()
    
def FrameSkipping(value=False):
    raise RuntimeError, "The FrameSkipping command is no longer needed and no longer supported" 
    #ReductionSingleton().set_frame_skipping(value)
    
def DarkCurrent(datafile):
    ReductionSingleton().set_dark_current_subtracter(sns_reduction_steps.SubtractDarkCurrent(datafile))

def TotalChargeNormalization():
    ReductionSingleton().set_normalizer(sns_reduction_steps.Normalize())
  
def MonitorNormalization():
    TotalChargeNormalization()
      
def BeamStopTransmission(normalize_to_unity=False, theta_dependent=False):
    ReductionSingleton().set_transmission(sns_reduction_steps.Transmission(normalize_to_unity=normalize_to_unity,
                                                                           theta_dependent=theta_dependent))
    
def BckBeamStopTransmission(normalize_to_unity=False, theta_dependent=False):
    ReductionSingleton().set_bck_transmission(sns_reduction_steps.Transmission(normalize_to_unity=normalize_to_unity,
                                                                               theta_dependent=theta_dependent))

def PerformFlightPathCorrection(do_correction=True):
    ReductionSingleton().get_data_loader().set_flight_path_correction(do_correction)
    
def SetTOFTailsCutoff(low_cut=0.0, high_cut=0.0):
    ReductionSingleton().get_data_loader().set_TOF_cuts(low_cut=low_cut, high_cut=high_cut)
    
def UseConfigTOFTailsCutoff(use_config=True):
    ReductionSingleton().get_data_loader().use_config_cuts(use_config)
    
def UseConfigMask(use_config=True):
    ReductionSingleton().get_data_loader().use_config_mask(use_config)