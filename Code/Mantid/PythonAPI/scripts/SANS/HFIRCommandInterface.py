"""
    Command interface for HFIR SANS instrument.
    
    As commands in the common CommandInterface module are found to be HFIR-specific, 
    they should be moved here.
"""
import HFIRInstrument
from CommandInterface import *

def HFIRSANS():
    Clear()
    ReductionSingleton().set_instrument(HFIRInstrument.HFIRSANS())
    SolidAngle()
    AzimuthalAverage()
    
def SetBckTransmission(trans, error):
    ReductionSingleton().set_bck_transmission(SANSReductionSteps.BaseTransmission(trans, error))

def BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0):
    ReductionSingleton().set_bck_transmission(SANSReductionSteps.DirectBeamTransmission(sample_file=sample_file,
                                                                                        empty_file=empty_file,
                                                                                        beam_radius=beam_radius))

def BckBeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0 ):
    ReductionSingleton().set_bck_transmission(SANSReductionSteps.BeamSpreaderTransmission(sample_spreader=sample_spreader, 
                                                                                          direct_spreader=direct_spreader,
                                                                                          sample_scattering=sample_scattering, 
                                                                                          direct_scattering=direct_scattering,
                                                                                          spreader_transmission=spreader_transmission, 
                                                                                          spreader_transmission_err=spreader_transmission_err))
    
def SetSampleDetectorOffset(distance):
    if not isinstance(ReductionSingleton().get_data_loader(), SANSReductionSteps.LoadRun):
        raise RuntimeError, "SetSampleDetectorOffset was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().get_data_loader().set_sample_detector_offset(distance)
    

def SetSampleDetectorDistance(distance):
    if not isinstance(ReductionSingleton().get_data_loader(), SANSReductionSteps.LoadRun):
        raise RuntimeError, "SetSampleDetectorDistance was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().get_data_loader().set_sample_detector_distance(distance)
    
def SetWavelength(wavelength, spread):
    if not isinstance(ReductionSingleton().instrument, HFIRInstrument.HFIRSANS):
        RuntimeError, "SetWavelength was called with the wrong instrument type: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().instrument.set_wavelength(wavelength, spread)
    
def ResetWavelength():
    """
        Resets the wavelength to the data file default
    """
    if not isinstance(ReductionSingleton().instrument, HFIRInstrument.HFIRSANS):
        RuntimeError, "SetWavelength was called with the wrong instrument type: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().instrument.set_wavelength()
    