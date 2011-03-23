"""
    List of common user commands for SANS 
"""
from reduction.command_interface import *
from sans_reducer import SANSReducer
import sans_reduction_steps
import hfir_instrument
import mantidsimple

## List of user commands ######################################################
def DirectBeamCenter(datafile):
    ReductionSingleton().set_beam_finder(sans_reduction_steps.DirectBeamCenter(datafile).set_masked_edges(1,1,1,1))

def ScatteringBeamCenter(datafile, beam_radius=3.0):
    ReductionSingleton().set_beam_finder(sans_reduction_steps.ScatteringBeamCenter(datafile, beam_radius=beam_radius).set_masked_edges(1,1,1,1))

def SetBeamCenter(x,y):
    ReductionSingleton().set_beam_finder(sans_reduction_steps.BaseBeamFinder(x,y))
    
def TimeNormalization():
    ReductionSingleton().set_normalizer(SANSReducer.NORMALIZATION_TIME)
    
def MonitorNormalization():
    ReductionSingleton().set_normalizer(SANSReducer.NORMALIZATION_MONITOR)    
    
def NoNormalization():
    ReductionSingleton().set_normalizer(None)
    
def SensitivityCorrection(flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None):
    ReductionSingleton().set_sensitivity_correcter(sans_reduction_steps.SensitivityCorrection(flood_data, 
                                                                                              min_sensitivity, 
                                                                                              max_sensitivity,
                                                                                              dark_current=dark_current))
def SetSensitivityBeamCenter(x,y):
    corr = ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.BaseBeamFinder(x,y))
    
def SensitivityDirectBeamCenter(datafile):
    ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.DirectBeamCenter(datafile).set_masked_edges(1,1,1,1))

def SensitivityScatteringBeamCenter(datafile, beam_radius=3.0):
    ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.ScatteringBeamCenter(datafile, beam_radius=beam_radius).set_masked_edges(1,1,1,1))
    
def NoSensitivityCorrection():
    ReductionSingleton().set_sensitivity_correcter(None)
    
def DarkCurrent(datafile):
    ReductionSingleton().set_dark_current_subtracter(sans_reduction_steps.SubtractDarkCurrent(datafile))
    
def NoDarkCurrent():
    ReductionSingleton().set_dark_current_subtracter(None)
    
def SolidAngle():
    ReductionSingleton().set_solid_angle_correcter(sans_reduction_steps.SolidAngle())
    #ReductionSingleton().set_solid_angle_correcter(mantidsimple.SANSSolidAngleCorrection, None, None)
    
def NoSolidAngle():
    ReductionSingleton().set_solid_angle_correcter(None)
    
def AzimuthalAverage(binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False):
    ReductionSingleton().set_azimuthal_averager(sans_reduction_steps.WeightedAzimuthalAverage(binning=binning,
                                                                                            suffix=suffix,
                                                                                            n_bins=n_bins,
                                                                                            n_subpix=n_subpix,
                                                                                            error_weighting=error_weighting,
                                                                                            log_binning=log_binning))

def NoTransmission():
    ReductionSingleton().set_transmission(None)
    
def SetTransmission(trans, error, theta_dependent=True):
    ReductionSingleton().set_transmission(sans_reduction_steps.BaseTransmission(trans, error, theta_dependent=theta_dependent))

def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True):
    ReductionSingleton().set_transmission(sans_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                    empty_file=empty_file,
                                                                                    beam_radius=beam_radius,
                                                                                    theta_dependent=theta_dependent))

def TransmissionDarkCurrent(dark_current=None):
    ReductionSingleton().get_transmission().set_dark_current(dark_current)

def ThetaDependentTransmission(theta_dependence=True):
    if ReductionSingleton().get_transmission() is None:
        raise RuntimeError, "A transmission algorithm must be selected before setting the theta-dependence of the correction."
    ReductionSingleton().get_transmission().set_theta_dependence(theta_dependence)

def BeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    ReductionSingleton().set_transmission(sans_reduction_steps.BeamSpreaderTransmission(sample_spreader=sample_spreader, 
                                                                                      direct_spreader=direct_spreader,
                                                                                      sample_scattering=sample_scattering, 
                                                                                      direct_scattering=direct_scattering,
                                                                                      spreader_transmission=spreader_transmission, 
                                                                                      spreader_transmission_err=spreader_transmission_err,
                                                                                      theta_dependent=theta_dependent))
  
def Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0): 
    ReductionSingleton().get_mask().mask_edges(nx_low=nx_low, nx_high=nx_high, ny_low=ny_low, ny_high=ny_high)

def MaskRectangle(x_min, x_max, y_min, y_max):
    ReductionSingleton().get_mask().add_pixel_rectangle(x_min, x_max, y_min, y_max)
    
def MaskDetectors(det_list):
    ReductionSingleton().get_mask().add_detector_list(det_list)

def Background(datafile):
    ReductionSingleton().set_background(datafile) 

def NoBackground():
    ReductionSingleton().set_background(None) 

def SaveIqAscii(reducer=None):
    if reducer is None:
        reducer = ReductionSingleton()
    reducer.set_save_Iq(sans_reduction_steps.SaveIqAscii())

def NoSaveIq():
    ReductionSingleton().set_save_Iq(None)
    
def SampleGeometry(shape):
    if not isinstance(ReductionSingleton().geometry_correcter, sans_reduction_steps.SampleGeomCor):
        defGeom = sans_reduction_steps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            sans_reduction_steps.SampleGeomCor(defGeom))
    
    ReductionSingleton().geometry_correcter.geo.shape = shape
    
def SampleThickness(thickness):
    if not isinstance(ReductionSingleton().geometry_correcter, sans_reduction_steps.SampleGeomCor):
        defGeom = sans_reduction_steps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            sans_reduction_steps.SampleGeomCor(defGeom))

    ReductionSingleton().geometry_correcter.geo.thickness = thickness
    
def SampleHeight(height):
    if not isinstance(ReductionSingleton().geometry_correcter, sans_reduction_steps.SampleGeomCor):
        defGeom = sans_reduction_steps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            sans_reduction_steps.SampleGeomCor(defGeom))
        
    ReductionSingleton().geometry_correcter.geo.height = height

def SampleWidth(width):
    if not isinstance(ReductionSingleton().geometry_correcter, sans_reduction_steps.SampleGeomCor):
        defGeom = sans_reduction_steps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            sans_reduction_steps.SampleGeomCor(defGeom))
        
    ReductionSingleton().geometry_correcter.geo.width = width


def HFIRSANS():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(hfir_instrument.HFIRSANS("GPSANS"))
    SolidAngle()
    AzimuthalAverage()
    
def BIOSANS():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(hfir_instrument.HFIRSANS("BIOSANS"))
    SolidAngle()
    AzimuthalAverage()

def GPSANS():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(hfir_instrument.HFIRSANS("GPSANS"))
    SolidAngle()
    AzimuthalAverage()
    
    
def SetBckTransmission(trans, error, theta_dependent=True):
    ReductionSingleton().set_bck_transmission(sans_reduction_steps.BaseTransmission(trans, error, theta_dependent=theta_dependent))

def BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True):
    ReductionSingleton().set_bck_transmission(sans_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                        empty_file=empty_file,
                                                                                        beam_radius=beam_radius,
                                                                                        theta_dependent=theta_dependent))

def BckBeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    ReductionSingleton().set_bck_transmission(sans_reduction_steps.BeamSpreaderTransmission(sample_spreader=sample_spreader, 
                                                                                          direct_spreader=direct_spreader,
                                                                                          sample_scattering=sample_scattering, 
                                                                                          direct_scattering=direct_scattering,
                                                                                          spreader_transmission=spreader_transmission, 
                                                                                          spreader_transmission_err=spreader_transmission_err,
                                                                                          theta_dependent=theta_dependent))

def BckTransmissionDarkCurrent(dark_current=None):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    ReductionSingleton().get_background().set_trans_dark_current(dark_current)

def BckThetaDependentTransmission(theta_dependence=True):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    ReductionSingleton().get_background().set_trans_theta_dependence(theta_dependence)
    
def SetSampleDetectorOffset(distance):
    if not isinstance(ReductionSingleton().get_data_loader(), sans_reduction_steps.LoadRun):
        raise RuntimeError, "SetSampleDetectorOffset was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().get_data_loader().set_sample_detector_offset(distance)
    

def SetSampleDetectorDistance(distance):
    if not isinstance(ReductionSingleton().get_data_loader(), sans_reduction_steps.LoadRun):
        raise RuntimeError, "SetSampleDetectorDistance was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().get_data_loader().set_sample_detector_distance(distance)
    
def SetWavelength(wavelength, spread):
    if not isinstance(ReductionSingleton().instrument, hfir_instrument.HFIRSANS):
        RuntimeError, "SetWavelength was called with the wrong instrument type: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().instrument.set_wavelength(wavelength, spread)
    
def ResetWavelength():
    """
        Resets the wavelength to the data file default
    """
    if not isinstance(ReductionSingleton().instrument, hfir_instrument.HFIRSANS):
        RuntimeError, "SetWavelength was called with the wrong instrument type: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().instrument.set_wavelength()
    
def IQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(sans_reduction_steps.IQxQy(nbins=nbins))
    
def NoIQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(None)
    