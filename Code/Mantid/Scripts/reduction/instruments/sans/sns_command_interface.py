"""
    Command set for EQSANS reduction
"""
from hfir_command_interface import *
from sns_reducer import EqSansReducer
import sns_instrument
import sns_reduction_steps

def EQSANS():
    Clear(EqSansReducer)
    ReductionSingleton().set_instrument(sns_instrument.EQSANS())
    NoSolidAngle()
    AzimuthalAverage()
    
def FrameSkipping(value):
    ReductionSingleton().set_frame_skipping(value)
    
def DarkCurrent(datafile):
    ReductionSingleton().set_dark_current_subtracter(sns_reduction_steps.SubtractDarkCurrent(datafile))

def MaskRectangle(x_min, x_max, y_min, y_max):
    ReductionSingleton().get_mask().add_pixel_rectangle(x_min, x_max, y_min, y_max)
    