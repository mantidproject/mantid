"""
    Command set for EQSANS reduction
"""
# Import the specific commands that we need
from reduction.command_interface import *
from hfir_command_interface import SetBeamCenter, ScatteringBeamCenter
from hfir_command_interface import NoDarkCurrent, NoNormalization, Mask, MaskDetectors, MaskRectangle
from hfir_command_interface import NoSensitivityCorrection, SensitivityCorrection
from hfir_command_interface import SolidAngle, NoSolidAngle, NoTransmission, SetTransmission
from hfir_command_interface import Background, NoBackground, IQxQy, NoIQxQy
from hfir_command_interface import NoSaveIq, DivideByThickness, BckDivideByThickness
from hfir_command_interface import ThetaDependentTransmission, BckThetaDependentTransmission, SetBckTransmission
from hfir_command_interface import TransmissionDarkCurrent, BckTransmissionDarkCurrent
from hfir_command_interface import SetDirectBeamAbsoluteScale, SetAbsoluteScale
from hfir_command_interface import SetSampleDetectorOffset, SetSampleDetectorDistance
from hfir_command_interface import SetSensitivityBeamCenter, SensitivityScatteringBeamCenter
from sns_reducer import EqSansReducer
import sns_instrument
import sans_reduction_steps
import sns_reduction_steps
from reduction.find_data import find_data

import mantidsimple

def EQSANS(keep_events=True):
    Clear(EqSansReducer)
    ReductionSingleton().set_instrument(sns_instrument.EQSANS())
    NoSolidAngle()
    AzimuthalAverage()
    ReductionSingleton().set_data_loader(sns_reduction_steps.LoadRun(keep_events=keep_events))
    
def FrameSkipping(value=False):
    raise RuntimeError, "The FrameSkipping command is no longer needed and no longer supported" 
    
def DarkCurrent(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_dark_current_subtracter(mantidsimple.EQSANSDarkCurrentSubtraction, 
                                                     InputWorkspace=None, Filename=datafile,
                                                     OutputWorkspace=None,
                                                     ReductionTableWorkspace=ReductionSingleton().get_reduction_table_name())

def TotalChargeNormalization(normalize_to_beam=True, beam_file=''):
    ReductionSingleton().set_normalizer(mantidsimple.EQSANSNormalise, None, NormaliseToBeam=normalize_to_beam, BeamSpectrumFile=beam_file)

def MonitorNormalization(normalize_to_beam=True, beam_file=''):
    TotalChargeNormalization(normalize_to_beam=normalize_to_beam, beam_file=beam_file)
      
def BeamStopTransmission(normalize_to_unity=True, theta_dependent=False):
    raise RuntimeError, "Transmission measurement using the beam stop hole is no longer supported" 
    
def BckBeamStopTransmission(normalize_to_unity=True, theta_dependent=False):
    raise RuntimeError, "Transmission measurement using the beam stop hole is no longer supported" 

def PerformFlightPathCorrection(do_correction=True):
    ReductionSingleton().get_data_loader().set_flight_path_correction(do_correction)
    
def SetTOFTailsCutoff(low_cut=0.0, high_cut=0.0):
    ReductionSingleton().get_data_loader().set_TOF_cuts(low_cut=low_cut, high_cut=high_cut)
    
def UseConfigTOFTailsCutoff(use_config=True):
    ReductionSingleton().get_data_loader().use_config_cuts(use_config)
    
def UseConfigMask(use_config=True):
    ReductionSingleton().get_data_loader().use_config_mask(use_config)
    
def SetWavelengthStep(step=0.1):
    ReductionSingleton().get_data_loader().set_wavelength_step(step)
    
def UseConfig(use_config=True):
    ReductionSingleton().get_data_loader().use_config(use_config)
    
def AzimuthalAverage(suffix="_Iq", n_bins=100, n_subpix=1, log_binning=False, scale=True):
    ReductionSingleton().set_azimuthal_averager(sns_reduction_steps.AzimuthalAverageByFrame(binning=None,
                                                                                            suffix=suffix,
                                                                                            n_bins=n_bins,
                                                                                            n_subpix=n_subpix,
                                                                                            error_weighting=False,
                                                                                            log_binning=log_binning,
                                                                                            scale=scale))
    
def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True, combine_frames=True):
    find_data(sample_file, instrument=ReductionSingleton().instrument.name())
    find_data(empty_file, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_transmission(sns_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                     empty_file=empty_file,
                                                                                     beam_radius=beam_radius,
                                                                                     theta_dependent=theta_dependent,
                                                                                     combine_frames=combine_frames,
                                                                                     use_sample_dc=True))

def BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True, combine_frames=True):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    find_data(sample_file, instrument=ReductionSingleton().instrument.name())
    find_data(empty_file, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().get_background().set_transmission(sns_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                                      empty_file=empty_file,
                                                                                                      beam_radius=beam_radius,
                                                                                                      theta_dependent=theta_dependent,
                                                                                                      combine_frames=combine_frames,
                                                                                                      use_sample_dc=True))
        
def CombineTransmissionFits(combine_frames):
    if not isinstance(ReductionSingleton().get_transmission(), sns_reduction_steps.DirectBeamTransmission):
        raise RuntimeError, "Trying to set transmission fitting option when the transmission calculation method hasn't been set correctly."
    ReductionSingleton().get_transmission().set_combine_frames(combine_frames)

def BckCombineTransmissionFits(combine_frames):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    if not isinstance(ReductionSingleton().get_background().get_transmission_calculator(), sns_reduction_steps.DirectBeamTransmission):
        raise RuntimeError, "Trying to set transmission fitting option when the transmission calculation method hasn't been set correctly."
    ReductionSingleton().get_background().get_transmission_calculator().set_combine_frames(combine_frames)
    
def IQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(mantidsimple.EQSANSQ2D, None, NumberOfBins=nbins)
    
def SaveIqAscii(process=None):
    ReductionSingleton().set_save_Iq(sns_reduction_steps.SaveIqAscii(process=process))
    
def DirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_beam_finder(sans_reduction_steps.DirectBeamCenter(datafile))

def SensitivityDirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.DirectBeamCenter(datafile))
    
def Resolution(sample_aperture_diameter=10.0):
    ReductionSingleton().get_azimuthal_averager().compute_resolution(sample_aperture_diameter=sample_aperture_diameter)
    
def IndependentBinning(independent_binning=True):
    ReductionSingleton().get_azimuthal_averager().use_independent_binning(independent_binning)
    