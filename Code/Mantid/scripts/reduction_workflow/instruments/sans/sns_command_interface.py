"""
    Command set for EQSANS reduction
"""
# Import the specific commands that we need
from reduction_workflow.command_interface import *

from hfir_command_interface import DarkCurrent, NoNormalization
from hfir_command_interface import SolidAngle, NoSolidAngle
from hfir_command_interface import SetBeamCenter, DirectBeamCenter, ScatteringBeamCenter
from hfir_command_interface import SetTransmission
from hfir_command_interface import SensitivityCorrection
from hfir_command_interface import IQxQy, NoIQxQy, SaveIq, NoSaveIq

from hfir_command_interface import Stitch

from mantid.api import AlgorithmManager
from mantid.kernel import Logger
import mantid.simpleapi as simpleapi
from reduction.find_data import find_data

def EQSANS(keep_events=False, property_manager=None):
    Clear()
    ReductionSingleton().set_instrument("EQSANS",
                                        "SetupEQSANSReduction",
                                        "HFIRSANSReduction")
    ReductionSingleton().reduction_properties["PreserveEvents"]=keep_events
    SolidAngle()
    AzimuthalAverage()
    if property_manager is not None:
        ReductionSingleton().set_reduction_table_name(property_manager)
    
def TotalChargeNormalization(normalize_to_beam=True, beam_file=''):
    if normalize_to_beam:
        ReductionSingleton().reduction_properties["Normalisation"]="BeamProfileAndCharge"
        ReductionSingleton().reduction_properties["MonitorReferenceFile"]=beam_file
    else:
        ReductionSingleton().reduction_properties["Normalisation"]="Charge"

def MonitorNormalization(normalize_to_beam=True, beam_file=''):
    ReductionSingleton().reduction_properties["Normalisation"]="Monitor"
    ReductionSingleton().reduction_properties["MonitorReferenceFile"]=beam_file
      
def BeamMonitorNormalization(reference_flux_file):
    find_data(reference_flux_file, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().get_data_loader().load_monitors(True)
    ReductionSingleton().set_normalizer(mantidsimple.EQSANSNormalise, None,
                                        BeamSpectrumFile=reference_flux_file,
                                        NormaliseToMonitor=True,
                                        ReductionProperties=ReductionSingleton().get_reduction_table_name())
    
def PerformFlightPathCorrection(do_correction=True):
    ReductionSingleton().reduction_properties["CorrectForFlightPath"]=do_correction
    
def SetTOFTailsCutoff(low_cut=0.0, high_cut=0.0):
    ReductionSingleton().reduction_properties["LowTOFCut"]=low_cut
    ReductionSingleton().reduction_properties["HighTOFCut"]=high_cut
    
def UseConfigTOFTailsCutoff(use_config=True):
    ReductionSingleton().reduction_properties["UseConfigTOFCuts"]=use_config
    
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
    
def Resolution(sample_aperture_diameter=10.0):
    ReductionSingleton().reduction_properties["ComputeResolution"]=True
    ReductionSingleton().reduction_properties["SampleApertureDiameter"]=sample_aperture_diameter
    
def IndependentBinning(independent_binning=True):
    ReductionSingleton().reduction_properties["IQIndependentBinning"]=independent_binning
    