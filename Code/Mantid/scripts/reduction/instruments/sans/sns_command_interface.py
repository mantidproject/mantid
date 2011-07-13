"""
    Command set for EQSANS reduction
"""
# Import the specific commands that we need
from reduction.command_interface import *
from hfir_command_interface import SetBeamCenter, DirectBeamCenter, ScatteringBeamCenter
from hfir_command_interface import NoDarkCurrent, NoNormalization, Mask, MaskDetectors, MaskRectangle
from hfir_command_interface import SensitivityCorrection, NoSensitivityCorrection
from hfir_command_interface import SolidAngle, NoSolidAngle, NoTransmission, SetTransmission#, DirectBeamTransmission
from hfir_command_interface import Background, NoBackground, IQxQy, NoIQxQy#, AzimuthalAverage
from hfir_command_interface import NoSaveIq#, SaveIqAscii 
from hfir_command_interface import ThetaDependentTransmission, BckThetaDependentTransmission, SetBckTransmission
from hfir_command_interface import TransmissionDarkCurrent, BckTransmissionDarkCurrent
from hfir_command_interface import SetDirectBeamAbsoluteScale, SetAbsoluteScale
from hfir_command_interface import SetSampleDetectorOffset, SetSampleDetectorDistance
from hfir_command_interface import SensitivityDirectBeamCenter, SetSensitivityBeamCenter, SensitivityScatteringBeamCenter
from sns_reducer import EqSansReducer
import sns_instrument
import sns_reduction_steps

import mantidsimple

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
      
def BeamStopTransmission(normalize_to_unity=True, theta_dependent=False):
    ReductionSingleton().set_transmission(sns_reduction_steps.BeamStopTransmission(normalize_to_unity=normalize_to_unity,
                                                                           theta_dependent=theta_dependent))
    
def BckBeamStopTransmission(normalize_to_unity=True, theta_dependent=False):
    ReductionSingleton().set_bck_transmission(sns_reduction_steps.BeamStopTransmission(normalize_to_unity=normalize_to_unity,
                                                                               theta_dependent=theta_dependent))

def PerformFlightPathCorrection(do_correction=True):
    ReductionSingleton().get_data_loader().set_flight_path_correction(do_correction)
    
def SetTOFTailsCutoff(low_cut=0.0, high_cut=0.0):
    ReductionSingleton().get_data_loader().set_TOF_cuts(low_cut=low_cut, high_cut=high_cut)
    
def UseConfigTOFTailsCutoff(use_config=True):
    ReductionSingleton().get_data_loader().use_config_cuts(use_config)
    
def UseConfigMask(use_config=True):
    ReductionSingleton().get_data_loader().use_config_mask(use_config)
    
def AzimuthalAverage(suffix="_Iq", n_bins=100, n_subpix=1, log_binning=False):
    ReductionSingleton().set_azimuthal_averager(sns_reduction_steps.AzimuthalAverageByFrame(binning=None,
                                                                                            suffix=suffix,
                                                                                            n_bins=n_bins,
                                                                                            n_subpix=n_subpix,
                                                                                            error_weighting=False,
                                                                                            log_binning=log_binning))
    
def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True, combine_frames=True):
    ReductionSingleton().set_transmission(sns_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                     empty_file=empty_file,
                                                                                     beam_radius=beam_radius,
                                                                                     theta_dependent=theta_dependent,
                                                                                     combine_frames=combine_frames))

def BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True, combine_frames=True):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    ReductionSingleton().get_background().set_transmission(sns_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                                      empty_file=empty_file,
                                                                                                      beam_radius=beam_radius,
                                                                                                      theta_dependent=theta_dependent,
                                                                                                      combine_frames=combine_frames))
        
def CombineTransmissionFits(combine_frames):
    if not isinstance(ReductionSingleton().get_transmission(), sns_reduction_steps.DirectBeamTransmission):
        raise RuntimeError, "Trying to see transmission fitting option when the transmission calculation method hasn't been set correctly."
    ReductionSingleton().get_transmission().set_combine_frames(combine_frames)

def BckCombineTransmissionFits(combine_frames):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    if not isinstance(ReductionSingleton().get_background().get_transmission_calculator(), sns_reduction_steps.DirectBeamTransmission):
        raise RuntimeError, "Trying to see transmission fitting option when the transmission calculation method hasn't been set correctly."
    ReductionSingleton().get_background().get_transmission_calculator().set_combine_frames(combine_frames)
    
def IQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(mantidsimple.EQSANSQ2D, None, NumberOfBins=nbins)
    
def SaveIqAscii():
    ReductionSingleton().set_save_Iq(sns_reduction_steps.SaveIqAscii())
    