"""
    List of common user commands for SANS 
"""
from reduction.command_interface import *
from sans_reducer import SANSReducer
import sans_reduction_steps
import hfir_load
import absolute_scale
import hfir_instrument
from reduction.find_data import find_data
import mantid.simpleapi as api

## List of user commands ######################################################
def DirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_beam_finder(sans_reduction_steps.DirectBeamCenter(datafile))

def ScatteringBeamCenter(datafile, beam_radius=3.0):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_beam_finder(sans_reduction_steps.ScatteringBeamCenter(datafile, beam_radius=beam_radius))

def SetBeamCenter(x,y):
    ReductionSingleton().set_beam_finder(sans_reduction_steps.BaseBeamFinder(x,y))
    
def TimeNormalization():
    ReductionSingleton().set_normalizer(sans_reduction_steps.Normalize(SANSReducer.NORMALIZATION_TIME))
    
def MonitorNormalization():
    ReductionSingleton().set_normalizer(sans_reduction_steps.Normalize(SANSReducer.NORMALIZATION_MONITOR))
    
def NoNormalization():
    ReductionSingleton().set_normalizer(None)
    
def SensitivityCorrection(flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, use_sample_dc=False):
    find_data(flood_data, instrument=ReductionSingleton().instrument.name())
    if dark_current is not None:
        dark_current=find_data(dark_current, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_sensitivity_correcter(sans_reduction_steps.SensitivityCorrection(flood_data, 
                                                                                              min_sensitivity, 
                                                                                              max_sensitivity,
                                                                                              dark_current=dark_current,
                                                                                              use_sample_dc=use_sample_dc))
def SetSensitivityBeamCenter(x,y):
    corr = ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.BaseBeamFinder(x,y))
    
def SensitivityDirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.DirectBeamCenter(datafile).set_persistent(False))

def SensitivityScatteringBeamCenter(datafile, beam_radius=3.0):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_sensitivity_beam_center(sans_reduction_steps.ScatteringBeamCenter(datafile, beam_radius=beam_radius).set_persistent(False))
    
def NoSensitivityCorrection():
    ReductionSingleton().set_sensitivity_correcter(None)
    
def DarkCurrent(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_dark_current_subtracter(api.HFIRDarkCurrentSubtraction, 
                                                     InputWorkspace=None, Filename=datafile,
                                                     OutputWorkspace=None,
                                                     ReductionProperties=ReductionSingleton().get_reduction_table_name())
    
def NoDarkCurrent():
    ReductionSingleton().set_dark_current_subtracter(None)
    
def SolidAngle(detector_tubes=False):
    ReductionSingleton().set_solid_angle_correcter(api.SANSSolidAngleCorrection, InputWorkspace=None, OutputWorkspace=None,
                                                   DetectorTubes=detector_tubes,
                                                   ReductionProperties=ReductionSingleton().get_reduction_table_name())
    
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
    find_data(sample_file, instrument=ReductionSingleton().instrument.name())
    find_data(empty_file, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_transmission(sans_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                    empty_file=empty_file,
                                                                                    beam_radius=beam_radius,
                                                                                    theta_dependent=theta_dependent))

def TransmissionDarkCurrent(dark_current=None):
    dark_current=find_data(dark_current, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().get_transmission().set_dark_current(dark_current)

def ThetaDependentTransmission(theta_dependence=True):
    if ReductionSingleton().get_transmission() is None:
        raise RuntimeError, "A transmission algorithm must be selected before setting the theta-dependence of the correction."
    ReductionSingleton().get_transmission().set_theta_dependence(theta_dependence)

def BeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    find_data(sample_spreader, instrument=ReductionSingleton().instrument.name())
    find_data(direct_spreader, instrument=ReductionSingleton().instrument.name())
    find_data(sample_scattering, instrument=ReductionSingleton().instrument.name())
    find_data(direct_scattering, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_transmission(sans_reduction_steps.BeamSpreaderTransmission(sample_spreader=sample_spreader, 
                                                                                      direct_spreader=direct_spreader,
                                                                                      sample_scattering=sample_scattering, 
                                                                                      direct_scattering=direct_scattering,
                                                                                      spreader_transmission=spreader_transmission, 
                                                                                      spreader_transmission_err=spreader_transmission_err,
                                                                                      theta_dependent=theta_dependent))

def SetTransmissionBeamCenter(x, y):
    if ReductionSingleton().get_transmission() is None:
        raise RuntimeError, "A transmission algorithm must be selected before setting the transmission beam center."
    ReductionSingleton().get_transmission().set_beam_finder(sans_reduction_steps.BaseBeamFinder(x,y))

def TransmissionDirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    if ReductionSingleton().get_transmission() is None:
        raise RuntimeError, "A transmission algorithm must be selected before setting the transmission beam center."
    ReductionSingleton().get_transmission().set_beam_finder(sans_reduction_steps.DirectBeamCenter(datafile).set_persistent(False))

def Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0): 
    ReductionSingleton().get_mask().mask_edges(nx_low=nx_low, nx_high=nx_high, ny_low=ny_low, ny_high=ny_high)

def MaskRectangle(x_min, x_max, y_min, y_max):
    ReductionSingleton().get_mask().add_pixel_rectangle(x_min, x_max, y_min, y_max)
    
def MaskDetectors(det_list):
    ReductionSingleton().get_mask().add_detector_list(det_list)

def Background(datafile):
    if type(datafile)==list:
        datafile=','.join(datafile)
        
    datafile = find_data(datafile, instrument=ReductionSingleton().instrument.name(), allow_multiple=True)
    ReductionSingleton().set_background(datafile) 

def NoBackground():
    ReductionSingleton().set_background(None) 

def SaveIqAscii(reducer=None, process=None):
    if reducer is None:
        reducer = ReductionSingleton()
    reducer.set_save_Iq(sans_reduction_steps.SaveIqAscii(process=process))

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
    ReductionSingleton().set_reduction(sans_reduction_steps.HFIRSetup())
    SolidAngle()
    AzimuthalAverage()
    
def BIOSANS():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(hfir_instrument.HFIRSANS("BIOSANS"))
    ReductionSingleton().set_reduction(sans_reduction_steps.HFIRSetup())
    SolidAngle()
    AzimuthalAverage()

def HFIRDEV():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(hfir_instrument.HFIRSANS("BIOSANS"))
    ReductionSingleton().set_reduction(sans_reduction_steps.HFIRSetup())
    SolidAngle()
    AzimuthalAverage()

def GPSANS():
    Clear(SANSReducer)
    ReductionSingleton().set_instrument(hfir_instrument.HFIRSANS("GPSANS"))
    ReductionSingleton().set_reduction(sans_reduction_steps.HFIRSetup())
    SolidAngle()
    AzimuthalAverage()
    
    
def SetBckTransmission(trans, error, theta_dependent=True):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    ReductionSingleton().get_background().set_transmission(sans_reduction_steps.BaseTransmission(trans, error, theta_dependent=theta_dependent))

def BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    find_data(sample_file, instrument=ReductionSingleton().instrument.name())
    find_data(empty_file, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().get_background().set_transmission(sans_reduction_steps.DirectBeamTransmission(sample_file=sample_file,
                                                                                        empty_file=empty_file,
                                                                                        beam_radius=beam_radius,
                                                                                        theta_dependent=theta_dependent))

def BckBeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    find_data(sample_spreader, instrument=ReductionSingleton().instrument.name())
    find_data(direct_spreader, instrument=ReductionSingleton().instrument.name())
    find_data(sample_scattering, instrument=ReductionSingleton().instrument.name())
    find_data(direct_scattering, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().get_background().set_transmission(sans_reduction_steps.BeamSpreaderTransmission(sample_spreader=sample_spreader, 
                                                                                          direct_spreader=direct_spreader,
                                                                                          sample_scattering=sample_scattering, 
                                                                                          direct_scattering=direct_scattering,
                                                                                          spreader_transmission=spreader_transmission, 
                                                                                          spreader_transmission_err=spreader_transmission_err,
                                                                                          theta_dependent=theta_dependent))

def SetBckTransmissionBeamCenter(x, y):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    if ReductionSingleton().get_background().get_transmission_calculator() is None:
        raise RuntimeError, "A transmission algorithm must be selected before setting the transmission beam center."
    ReductionSingleton().get_background().get_transmission_calculator().set_beam_finder(sans_reduction_steps.BaseBeamFinder(x,y))

def BckTransmissionDirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().instrument.name())
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    if ReductionSingleton().get_background().get_transmission_calculator() is None:
        raise RuntimeError, "A transmission algorithm must be selected before setting the transmission beam center."
    ReductionSingleton().get_background().get_transmission_calculator().set_beam_finder(sans_reduction_steps.DirectBeamCenter(datafile).set_persistent(False))


def BckTransmissionDarkCurrent(dark_current=None):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    dark_current=find_data(dark_current, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().get_background().set_trans_dark_current(dark_current)

def BckThetaDependentTransmission(theta_dependence=True):
    if ReductionSingleton().get_background() is None:
        raise RuntimeError, "A background hasn't been defined."
    ReductionSingleton().get_background().set_trans_theta_dependence(theta_dependence)
    
def SetSampleDetectorOffset(distance):
    ReductionSingleton().get_data_loader().set_sample_detector_offset(distance)
    

def SetSampleDetectorDistance(distance):
    ReductionSingleton().get_data_loader().set_sample_detector_distance(distance)
    
def SetWavelength(wavelength, spread):
    if not isinstance(ReductionSingleton().get_data_loader(), hfir_load.LoadRun):
        raise RuntimeError, "SetWavelength was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().get_data_loader().set_wavelength(wavelength, spread)
    
def MaskDetectorSide(side_to_mask=None):
    if not isinstance(ReductionSingleton().get_data_loader(), hfir_load.LoadRun):
        raise RuntimeError, "MaskDetectorSide was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    if side_to_mask.lower() == "front":
        side_to_mask = 0
    elif side_to_mask.lower() == "back":
        side_to_mask = 1
    ReductionSingleton().get_data_loader().mask_detector_side(side_to_mask)
    
def ResetWavelength():
    """
        Resets the wavelength to the data file default
    """
    if not isinstance(ReductionSingleton().get_data_loader(), hfir_load.LoadRun):
        raise RuntimeError, "ResetWavelength was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    ReductionSingleton().get_data_loader().set_wavelength()
    
def IQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(api.EQSANSQ2D, InputWorkspace=None, 
                                   NumberOfBins=nbins)
    
def NoIQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(None)
    
def SetAbsoluteScale(factor):
    ReductionSingleton().set_absolute_scale(absolute_scale.BaseAbsoluteScale(factor))
    
def SetDirectBeamAbsoluteScale(direct_beam, beamstop_diameter=None, attenuator_trans=1.0, sample_thickness=None, apply_sensitivity=False):
    if sample_thickness is not None:
        print "sample_thickness is no longer used with SetDirectBeamAbsoluteScale: use DivideByThickness"
    find_data(direct_beam, instrument=ReductionSingleton().instrument.name())
    ReductionSingleton().set_absolute_scale(absolute_scale.AbsoluteScale(data_file=direct_beam, 
                                                                         beamstop_diameter=beamstop_diameter, 
                                                                         attenuator_trans=attenuator_trans, 
                                                                         apply_sensitivity=apply_sensitivity))
   
def DivideByThickness(thickness=1.0):
    if thickness is None or thickness == 1.0:
        ReductionSingleton().set_geometry_correcter(None)
    else:
        ReductionSingleton().set_geometry_correcter(api.NormaliseByThickness, InputWorkspace=None, OutputWorkspace=None, SampleThickness=thickness)
        
def BckDivideByThickness(thickness=1.0):
    print "Background thickness can no longer be set: only the final sample-minus-data workspace can be divided by the sample thickness."

def Stitch(data_list=[], q_min=None, q_max=None, output_workspace=None,
           scale=None, save_output=False):
    from LargeScaleStructures.data_stitching import stitch
    stitch(data_list, q_min=q_min, q_max=q_max, scale=scale, save_output=save_output)