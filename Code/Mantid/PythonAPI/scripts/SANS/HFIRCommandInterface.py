"""
    Command interface for HFIR SANS instrument.
    
    As commands in the common CommandInterface module are found to be HFIR-specific, 
    they should be moved here.
"""
import HFIRInstrument
from CommandInterface import *

def HFIRSANS():
    ReductionSingleton().set_instrument(HFIRInstrument.HFIRSANS())
    SolidAngle()
    AzimuthalAverage()
    
    
