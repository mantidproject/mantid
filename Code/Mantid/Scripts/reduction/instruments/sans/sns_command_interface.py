"""
    Command set for EQSANS reduction
"""
# Import the specific commands that we need
from reduction.command_interface import *
from hfir_command_interface import SetBeamCenter, DirectBeamCenter, ScatteringBeamCenter
from hfir_command_interface import NoDarkCurrent, NoNormalization, Mask
from hfir_command_interface import SensitivityCorrection, NoSensitivityCorrection
from hfir_command_interface import SolidAngle, NoSolidAngle, NoTransmission
from hfir_command_interface import Background, NoBackground, AzimuthalAverage
from hfir_command_interface import SaveIqAscii, NoSaveIq
from sns_reducer import EqSansReducer
import sns_instrument
import sns_reduction_steps

def EQSANS():
    Clear(EqSansReducer)
    ReductionSingleton().set_instrument(sns_instrument.EQSANS())
    NoSolidAngle()
    AzimuthalAverage()
    
def FrameSkipping(value=False):
    ReductionSingleton().set_frame_skipping(value)
    
def DarkCurrent(datafile):
    ReductionSingleton().set_dark_current_subtracter(sns_reduction_steps.SubtractDarkCurrent(datafile))

def MaskRectangle(x_min, x_max, y_min, y_max):
    ReductionSingleton().get_mask().add_pixel_rectangle(x_min, x_max, y_min, y_max)
    
def TotalChargeNormalization():
    ReductionSingleton().set_normalizer(sns_reduction_steps.Normalize())
    
def MeasureTransmission(normalize_to_unity=False):
    ReductionSingleton().set_transmission(sns_reduction_steps.Transmission(normalize_to_unity=normalize_to_unity))