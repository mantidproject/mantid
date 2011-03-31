"""
    Command set for NIST SANS reduction
"""
# Import the specific commands that we need
from hfir_command_interface import *
from sans_reducer import SANSReducer
import ngsans_instrument

def NGSANS():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(ngsans_instrument.NGSANS())
    ReductionSingleton()._data_loader = ngsans_instrument.NGSANSLoadRun()
    SolidAngle()
    MonitorNormalization()
    AzimuthalAverage()
    
