"""
    Command set for EQSANS reduction
"""
from hfir_command_interface import *
from sns_reducer import EqSansReducer
import sns_instrument

def EQSANS():
    Clear(EqSansReducer)
    ReductionSingleton().set_instrument(sns_instrument.EQSANS())
    NoSolidAngle()
    AzimuthalAverage()
    
def AzimuthalAverage(binning="0.001,0.001,0.05", suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1):
    ReductionSingleton().set_azimuthal_averager(sans_reduction_steps.WeightedAzimuthalAverage(binning=binning,
                                                                                            suffix=suffix,
                                                                                            n_bins=n_bins,
                                                                                            n_subpix=n_subpix,
                                                                                            error_weighting=error_weighting))
    
def FrameSkipping(value):
    ReductionSingleton().set_frame_skipping(value)