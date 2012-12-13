"""
    List of common user commands for HFIR SANS 
"""
from reduction_workflow.command_interface import *
from reduction_workflow.find_data import *
from mantid.api import AlgorithmManager
    
import mantid.simpleapi as simpleapi

## List of user commands ######################################################
def BIOSANS():
    Clear()
    ReductionSingleton().set_instrument("BIOSANS",
                                        "SetupHFIRReduction",
                                        "HFIRSANSReduction")
    TimeNormalization()
    SolidAngle()
    AzimuthalAverage()

def GPSANS():
    Clear()
    ReductionSingleton().set_instrument("GPSANS", 
                                        "SetupHFIRReduction",
                                        "HFIRSANSReduction")
    TimeNormalization()
    SolidAngle()
    AzimuthalAverage()

def DirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BeamCenterMethod"]="DirectBeam"
    ReductionSingleton().reduction_properties["BeamCenterFile"]=datafile
    ReductionSingleton().reduction_properties["BeamCenterPersistent"]=True

def ScatteringBeamCenter(datafile, beam_radius=3.0):
    find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BeamCenterMethod"]="Scattering"
    ReductionSingleton().reduction_properties["BeamRadius"]=beam_radius
    ReductionSingleton().reduction_properties["BeamCenterFile"]=datafile
    ReductionSingleton().reduction_properties["BeamCenterPersistent"]=True

def SetBeamCenter(x,y):
    ReductionSingleton().reduction_properties["BeamCenterMethod"]="Value"
    ReductionSingleton().reduction_properties["BeamCenterX"]=x
    ReductionSingleton().reduction_properties["BeamCenterY"]=y
    
def TimeNormalization():
    ReductionSingleton().reduction_properties["Normalisation"]="Timer"
    
def MonitorNormalization():
    ReductionSingleton().reduction_properties["Normalisation"]="Monitor"
    
def NoNormalization():
    ReductionSingleton().reduction_properties["Normalisation"]="None"
    
def SensitivityCorrection(flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, use_sample_dc=False):
    flood_data = find_data(flood_data, instrument=ReductionSingleton().get_instrument())
    if dark_current is not None:
        dark_current = find_data(dark_current, instrument=ReductionSingleton().get_instrument())
    
    ReductionSingleton().reduction_properties["SensitivityFile"] = flood_data
    ReductionSingleton().reduction_properties["MinEfficiency"] = min_sensitivity
    ReductionSingleton().reduction_properties["MaxEfficiency"] = max_sensitivity
    if dark_current is not None:
        ReductionSingleton().reduction_properties["SensitivityDarkCurrentFile"] = dark_current
    elif ReductionSingleton().reduction_properties.has_key("SensitivityDarkCurrentFile"):
        del ReductionSingleton().reduction_properties["SensitivityDarkCurrentFile"]
    if ReductionSingleton().reduction_properties.has_key("SensitivityBeamCenterX"):
        del ReductionSingleton().reduction_properties["SensitivityBeamCenterX"]
    if ReductionSingleton().reduction_properties.has_key("SensitivityBeamCenterY"):
        del ReductionSingleton().reduction_properties["SensitivityBeamCenterY"]
    ReductionSingleton().reduction_properties["UseDefaultDC"] = use_sample_dc
        
def SetSensitivityBeamCenter(x,y):
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"]="Value"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterX"] = x
    ReductionSingleton().reduction_properties["SensitivityBeamCenterY"] = y
    
def SensitivityDirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"]="DirectBeam"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterFile"]=datafile

def SensitivityScatteringBeamCenter(datafile, beam_radius=3.0):
    find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"]="Scattering"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterRadius"]=beam_radius
    ReductionSingleton().reduction_properties["SensitivityBeamCenterFile"]=datafile
    
def NoSensitivityCorrection():
    ReductionSingleton().reduction_properties["SensitivityFile"] = None
    
def DarkCurrent(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["DarkCurrentFile"] = datafile
    
def NoDarkCurrent():
    ReductionSingleton().reduction_properties["DarkCurrentFile"] = None
    
def SolidAngle(detector_tubes=False):
    ReductionSingleton().reduction_properties["SolidAngleCorrection"]=True
    ReductionSingleton().reduction_properties["DetectorTubes"]=detector_tubes
    
def NoSolidAngle():
    ReductionSingleton().reduction_properties["SolidAngleCorrection"]=False
    
def AzimuthalAverage(binning=None, suffix="_Iq", error_weighting=False, 
                     n_bins=100, n_subpix=1, log_binning=False):
    # Suffix is no longer used but kept for backward compatibility 
    ReductionSingleton().reduction_properties["DoAzimuthalAverage"]=True
    ReductionSingleton().reduction_properties["IQBinning"]=binning
    ReductionSingleton().reduction_properties["IQNumberOfBins"]=n_bins
    ReductionSingleton().reduction_properties["IQLogBinning"]=log_binning
    ReductionSingleton().reduction_properties["NumberOfSubpixels"]=n_subpix
    ReductionSingleton().reduction_properties["ErrorWeighting"]=error_weighting

def NoTransmission():
    if ReductionSingleton().reduction_properties.has_key("TransmissionValue"):
        del ReductionSingleton().reduction_properties["TransmissionValue"]
    if ReductionSingleton().reduction_properties.has_key("TransmissionError"):
        del ReductionSingleton().reduction_properties["TransmissionError"]
    if ReductionSingleton().reduction_properties.has_key("TransmissionMethod"):
        del ReductionSingleton().reduction_properties["TransmissionMethod"]
    if ReductionSingleton().reduction_properties.has_key("TransmissionBeamRadius"):
        del ReductionSingleton().reduction_properties["TransmissionBeamRadius"]
    if ReductionSingleton().reduction_properties.has_key("TransmissionSampleDataFile"):
        del ReductionSingleton().reduction_properties["TransmissionSampleDataFile"]
    if ReductionSingleton().reduction_properties.has_key("TransmissionEmptyDataFile"):
        del ReductionSingleton().reduction_properties["TransmissionEmptyDataFile"]
    if ReductionSingleton().reduction_properties.has_key("ThetaDependentTransmission"):
        del ReductionSingleton().reduction_properties["ThetaDependentTransmission"]
    
def SetTransmission(trans, error, theta_dependent=True):
    ReductionSingleton().reduction_properties["TransmissionMethod"] = "Value"
    ReductionSingleton().reduction_properties["TransmissionValue"] = trans
    ReductionSingleton().reduction_properties["TransmissionError"] = error
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependent

def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True):
    find_data(sample_file, instrument=ReductionSingleton().get_instrument())
    find_data(empty_file, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["TransmissionMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["TransmissionBeamRadius"] = beam_radius
    ReductionSingleton().reduction_properties["TransmissionSampleDataFile"] = sample_file
    ReductionSingleton().reduction_properties["TransmissionEmptyDataFile"] = empty_file
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependent

def TransmissionDarkCurrent(dark_current=None):
    if dark_current is not None:
        find_data(dark_current, instrument=ReductionSingleton().get_instrument())
        ReductionSingleton().reduction_properties["TransmissionDarkCurrentFile"] = dark_current
    elif ReductionSingleton().reduction_properties.has_key("TransmissionDarkCurrentFile"):
        del ReductionSingleton().reduction_properties["TransmissionDarkCurrentFile"]

def ThetaDependentTransmission(theta_dependence=True):
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependence

def BeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    find_data(sample_spreader, instrument=ReductionSingleton().get_instrument())
    find_data(direct_spreader, instrument=ReductionSingleton().get_instrument())
    find_data(sample_scattering, instrument=ReductionSingleton().get_instrument())
    find_data(direct_scattering, instrument=ReductionSingleton().get_instrument())
    
    ReductionSingleton().reduction_properties["TransmissionMethod"] = "BeamSpreader"
    ReductionSingleton().reduction_properties["TransSampleSpreaderFilename"] = sample_spreader
    ReductionSingleton().reduction_properties["TransDirectSpreaderFilename"] = direct_spreader
    ReductionSingleton().reduction_properties["TransSampleScatteringFilename"] = sample_scattering
    ReductionSingleton().reduction_properties["TransDirectScatteringFilename"] = direct_scattering
    ReductionSingleton().reduction_properties["SpreaderTransmissionValue"] = spreader_transmission
    ReductionSingleton().reduction_properties["SpreaderTransmissionError"] = spreader_transmission_err
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependent

def SetTransmissionBeamCenter(x, y):
    ReductionSingleton().reduction_properties["TransmissionBeamCenterMethod"] = "Value"
    ReductionSingleton().reduction_properties["TransmissionBeamCenterX"] = x
    ReductionSingleton().reduction_properties["TransmissionBeamCenterY"] = y

def TransmissionDirectBeamCenter(datafile):
    find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["TransmissionBeamCenterMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["TransmissionBeamCenterFile"] = datafile

def Background(datafile):
    if type(datafile)==list:
        datafile=','.join(datafile)
    find_data(datafile, instrument=ReductionSingleton().get_instrument(), allow_multiple=True)
    ReductionSingleton().reduction_properties["BackgroundFiles"] = datafile

def NoBackground():
    ReductionSingleton().reduction_properties["BackgroundFiles"] = ""

def NoBckTransmission():
    if ReductionSingleton().reduction_properties.has_key("BckTransmissionValue"):
        del ReductionSingleton().reduction_properties["BckTransmissionValue"]
    if ReductionSingleton().reduction_properties.has_key("BckTransmissionError"):
        del ReductionSingleton().reduction_properties["BckTransmissionError"]
    if ReductionSingleton().reduction_properties.has_key("BckTransmissionMethod"):
        del ReductionSingleton().reduction_properties["BckTransmissionMethod"]
    if ReductionSingleton().reduction_properties.has_key("BckTransmissionBeamRadius"):
        del ReductionSingleton().reduction_properties["BckTransmissionBeamRadius"]
    if ReductionSingleton().reduction_properties.has_key("BckTransmissionSampleDataFile"):
        del ReductionSingleton().reduction_properties["BckTransmissionSampleDataFile"]
    if ReductionSingleton().reduction_properties.has_key("BckTransmissionEmptyDataFile"):
        del ReductionSingleton().reduction_properties["BckTransmissionEmptyDataFile"]
    if ReductionSingleton().reduction_properties.has_key("BckThetaDependentTransmission"):
        del ReductionSingleton().reduction_properties["BckThetaDependentTransmission"]
    
def SetBckTransmission(trans, error, theta_dependent=True):
    ReductionSingleton().reduction_properties["BckTransmissionMethod"] = "Value"
    ReductionSingleton().reduction_properties["BckTransmissionValue"] = trans
    ReductionSingleton().reduction_properties["BckTransmissionError"] = error
    ReductionSingleton().reduction_properties["BckThetaDependentTransmission"] = theta_dependent

def BckDirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True):
    find_data(sample_file, instrument=ReductionSingleton().get_instrument())
    find_data(empty_file, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BckTransmissionMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["BckTransmissionBeamRadius"] = beam_radius
    ReductionSingleton().reduction_properties["BckTransmissionSampleDataFile"] = sample_file
    ReductionSingleton().reduction_properties["BckTransmissionEmptyDataFile"] = empty_file
    ReductionSingleton().reduction_properties["BckThetaDependentTransmission"] = theta_dependent

def BckBeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    find_data(sample_spreader, instrument=ReductionSingleton().get_instrument())
    find_data(direct_spreader, instrument=ReductionSingleton().get_instrument())
    find_data(sample_scattering, instrument=ReductionSingleton().get_instrument())
    find_data(direct_scattering, instrument=ReductionSingleton().get_instrument())
    
    ReductionSingleton().reduction_properties["BckTransmissionMethod"] = "BeamSpreader"
    ReductionSingleton().reduction_properties["BckTransSampleSpreaderFilename"] = sample_spreader
    ReductionSingleton().reduction_properties["BckTransDirectSpreaderFilename"] = direct_spreader
    ReductionSingleton().reduction_properties["BckTransSampleScatteringFilename"] = sample_scattering
    ReductionSingleton().reduction_properties["BckTransDirectScatteringFilename"] = direct_scattering
    ReductionSingleton().reduction_properties["BckSpreaderTransmissionValue"] = spreader_transmission
    ReductionSingleton().reduction_properties["BckSpreaderTransmissionError"] = spreader_transmission_err
    ReductionSingleton().reduction_properties["BckThetaDependentTransmission"] = theta_dependent

def SetBckTransmissionBeamCenter(x, y):
    ReductionSingleton().reduction_properties["BckTransmissionBeamCenterMethod"] = "Value"
    ReductionSingleton().reduction_properties["BckTransmissionBeamCenterX"] = x
    ReductionSingleton().reduction_properties["BckTransmissionBeamCenterY"] = y

def BckTransmissionDirectBeamCenter(datafile):
    ReductionSingleton().reduction_properties["BckTransmissionBeamCenterMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["BckTransmissionBeamCenterFile"]=datafile

def BckTransmissionDarkCurrent(dark_current=None):
    if dark_current is not None:
        find_data(dark_current, instrument=ReductionSingleton().get_instrument())
        ReductionSingleton().reduction_properties["BckTransmissionDarkCurrentFile"] = dark_current
    elif ReductionSingleton().reduction_properties.has_key("BckTransmissionDarkCurrentFile"):
        del ReductionSingleton().reduction_properties["BckTransmissionDarkCurrentFile"]

def BckThetaDependentTransmission(theta_dependence=True):
    ReductionSingleton().reduction_properties["BckThetaDependentTransmission"] = theta_dependence
    
def SetSampleDetectorOffset(distance):
    ReductionSingleton().reduction_properties["SampleDetectorDistanceOffset"] = distance

def SetSampleDetectorDistance(distance):
    ReductionSingleton().reduction_properties["SampleDetectorDistance"] = distance
    
def SetWavelength(wavelength, spread):
    ReductionSingleton().reduction_properties["Wavelength"] = wavelength
    ReductionSingleton().reduction_properties["WavelengthSpread"] = spread
    
def ResetWavelength():
    """ Resets the wavelength to the data file default """
    ReductionSingleton().reduction_properties["Wavelength"] = None
    ReductionSingleton().reduction_properties["WavelengthSpread"] = None
    
def SaveIq(output_dir='', process=''):
    ReductionSingleton().reduction_properties["OutputDirectory"] = output_dir
    ReductionSingleton().reduction_properties["ProcessInfo"] = process

def NoSaveIq():
        if ReductionSingleton().reduction_properties.has_key("OutputDirectory"):
            del ReductionSingleton().reduction_properties["OutputDirectory"]
            
def IQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(mantidsimple.EQSANSQ2D, InputWorkspace=None, 
                                   NumberOfBins=nbins)
    
def NoIQxQy(nbins=100):
    ReductionSingleton().set_IQxQy(None)
    
def Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0): 
    #TODO
    ReductionSingleton().get_mask().mask_edges(nx_low=nx_low, nx_high=nx_high, ny_low=ny_low, ny_high=ny_high)

def MaskRectangle(x_min, x_max, y_min, y_max):
    #TODO
    ReductionSingleton().get_mask().add_pixel_rectangle(x_min, x_max, y_min, y_max)
    
def MaskDetectors(det_list):
    #TODO
    ReductionSingleton().get_mask().add_detector_list(det_list)
    
def MaskDetectorSide(side_to_mask=None):
    if not isinstance(ReductionSingleton().get_data_loader(), hfir_load.LoadRun):
        raise RuntimeError, "MaskDetectorSide was called with the wrong data loader: re-initialize your instrument (e.g. HFIRSANS() )"    
    if side_to_mask.lower() == "front":
        side_to_mask = 0
    elif side_to_mask.lower() == "back":
        side_to_mask = 1
    ReductionSingleton().get_data_loader().mask_detector_side(side_to_mask)
    
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
        if ReductionSingleton().reduction_properties.has_key("SampleThickness"):
            del ReductionSingleton().reduction_properties["SampleThickness"]
    else:
        ReductionSingleton().reduction_properties["SampleThickness"] = thickness
        
def Stitch(data_list=[], q_min=None, q_max=None, scale=None, save_output=False):
    from LargeScaleStructures.data_stitching import stitch
    stitch(data_list, q_min=q_min, q_max=q_max, scale=scale, save_output=save_output)