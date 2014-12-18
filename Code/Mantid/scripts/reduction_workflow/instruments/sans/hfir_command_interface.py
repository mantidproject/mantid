"""
    List of common user commands for HFIR SANS
"""
from reduction_workflow.command_interface import *
from reduction_workflow.find_data import *
from reduction_workflow.instruments.sans import hfir_instrument
from mantid.api import AlgorithmManager
from mantid.kernel import Logger
import mantid.simpleapi as simpleapi
import os

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

def DataPath(path):
    ReductionSingleton().set_data_path(path)
    ReductionSingleton().set_output_path(path)
    ReductionSingleton().reduction_properties["OutputDirectory"] = path

def DirectBeamCenter(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BeamCenterMethod"]="DirectBeam"
    ReductionSingleton().reduction_properties["BeamCenterFile"]=datafile

def ScatteringBeamCenter(datafile, beam_radius=3.0):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BeamCenterMethod"]="Scattering"
    ReductionSingleton().reduction_properties["BeamRadius"]=beam_radius
    ReductionSingleton().reduction_properties["BeamCenterFile"]=datafile

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
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"]="DirectBeam"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterFile"]=datafile

def SensitivityScatteringBeamCenter(datafile, beam_radius=3.0):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"]="Scattering"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterRadius"]=beam_radius
    ReductionSingleton().reduction_properties["SensitivityBeamCenterFile"]=datafile

def NoSensitivityCorrection():
    if ReductionSingleton().reduction_properties.has_key("SensitivityFile"):
        del ReductionSingleton().reduction_properties["SensitivityFile"]

def DarkCurrent(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["DarkCurrentFile"] = datafile

def NoDarkCurrent():
    if ReductionSingleton().reduction_properties.has_key("DarkCurrentFile"):
        del ReductionSingleton().reduction_properties["DarkCurrentFile"]

def SolidAngle(detector_tubes=False):
    ReductionSingleton().reduction_properties["SolidAngleCorrection"]=True
    ReductionSingleton().reduction_properties["DetectorTubes"]=detector_tubes

def NoSolidAngle():
    ReductionSingleton().reduction_properties["SolidAngleCorrection"]=False

def AzimuthalAverage(binning=None, suffix="_Iq", error_weighting=False,
                     n_bins=100, n_subpix=1, log_binning=False):
    # Suffix is no longer used but kept for backward compatibility
    ReductionSingleton().reduction_properties["DoAzimuthalAverage"]=True
    if binning is not None:
        ReductionSingleton().reduction_properties["IQBinning"]=binning
    elif ReductionSingleton().reduction_properties.has_key("IQBinning"):
        del ReductionSingleton().reduction_properties["IQBinning"]
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

def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True,
                           use_sample_dc=True):
    sample_file = find_data(sample_file, instrument=ReductionSingleton().get_instrument())
    empty_file = find_data(empty_file, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["TransmissionMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["TransmissionBeamRadius"] = beam_radius
    ReductionSingleton().reduction_properties["TransmissionSampleDataFile"] = sample_file
    ReductionSingleton().reduction_properties["TransmissionEmptyDataFile"] = empty_file
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependent
    ReductionSingleton().reduction_properties["TransmissionUseSampleDC"] = use_sample_dc

def TransmissionDarkCurrent(dark_current=None):
    if dark_current is not None:
        dark_current = find_data(dark_current, instrument=ReductionSingleton().get_instrument())
        ReductionSingleton().reduction_properties["TransmissionDarkCurrentFile"] = dark_current
    elif ReductionSingleton().reduction_properties.has_key("TransmissionDarkCurrentFile"):
        del ReductionSingleton().reduction_properties["TransmissionDarkCurrentFile"]

def ThetaDependentTransmission(theta_dependence=True):
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependence

def BeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    sample_spreader = find_data(sample_spreader, instrument=ReductionSingleton().get_instrument())
    direct_spreader = find_data(direct_spreader, instrument=ReductionSingleton().get_instrument())
    sample_scattering = find_data(sample_scattering, instrument=ReductionSingleton().get_instrument())
    direct_scattering = find_data(direct_scattering, instrument=ReductionSingleton().get_instrument())

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
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
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
    sample_file = find_data(sample_file, instrument=ReductionSingleton().get_instrument())
    empty_file = find_data(empty_file, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BckTransmissionMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["BckTransmissionBeamRadius"] = beam_radius
    ReductionSingleton().reduction_properties["BckTransmissionSampleDataFile"] = sample_file
    ReductionSingleton().reduction_properties["BckTransmissionEmptyDataFile"] = empty_file
    ReductionSingleton().reduction_properties["BckThetaDependentTransmission"] = theta_dependent

def BckBeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0,
                             theta_dependent=True ):
    sample_spreader = find_data(sample_spreader, instrument=ReductionSingleton().get_instrument())
    direct_spreader = find_data(direct_spreader, instrument=ReductionSingleton().get_instrument())
    sample_scattering = find_data(sample_scattering, instrument=ReductionSingleton().get_instrument())
    direct_scattering = find_data(direct_scattering, instrument=ReductionSingleton().get_instrument())

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
        dark_current = find_data(dark_current, instrument=ReductionSingleton().get_instrument())
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
    if ReductionSingleton().reduction_properties.has_key("Wavelength"):
        del ReductionSingleton().reduction_properties["Wavelength"]
    if ReductionSingleton().reduction_properties.has_key("WavelengthSpread"):
        del ReductionSingleton().reduction_properties["WavelengthSpread"]

def SaveIqAscii(reducer=None, process=''):
    """ Old command for backward compatibility """
    msg = "SaveIqAscii is not longer used:\n  "
    msg += "Please use 'SaveIq' instead\n  "
    Logger("CommandInterface").warning(msg)
    ReductionSingleton().reduction_properties["ProcessInfo"] = str(process)

def SaveIq(output_dir=None, process=''):
    if output_dir is not None:
        ReductionSingleton().reduction_properties["OutputDirectory"] = output_dir
    ReductionSingleton().reduction_properties["ProcessInfo"] = process

def NoSaveIq():
    if ReductionSingleton().reduction_properties.has_key("ProcessInfo"):
        del ReductionSingleton().reduction_properties["ProcessInfo"]

def IQxQy(nbins=100):
    ReductionSingleton().reduction_properties["Do2DReduction"] = True
    ReductionSingleton().reduction_properties["IQ2DNumberOfBins"] = nbins

def NoIQxQy():
    ReductionSingleton().reduction_properties["Do2DReduction"] = False

def Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0):
    ReductionSingleton().reduction_properties["MaskedEdges"] = [nx_low, nx_high,
                                                                ny_low, ny_high]

def MaskRectangle(x_min, x_max, y_min, y_max):
    masked_pixels = []
    for ix in range(x_min, x_max+1):
        for iy in range(y_min, y_max+1):
            masked_pixels.append([ix, iy])
    det_list = hfir_instrument.get_detector_from_pixel(masked_pixels)
    MaskDetectors(det_list)

def MaskDetectors(det_list):
    if ReductionSingleton().reduction_properties.has_key("MaskedDetectorList"):
        ReductionSingleton().reduction_properties["MaskedDetectorList"].extend(det_list)
    else:
        ReductionSingleton().reduction_properties["MaskedDetectorList"] = det_list

def MaskDetectorSide(side_to_mask=None):
    if side_to_mask is None:
        if ReductionSingleton().reduction_properties.has_key("MaskedSide"):
            del ReductionSingleton().reduction_properties["MaskedSide"]
    else:
        ReductionSingleton().reduction_properties["MaskedSide"] = side_to_mask

def SetAbsoluteScale(factor):
    ReductionSingleton().reduction_properties["AbsoluteScaleMethod"] = "Value"
    ReductionSingleton().reduction_properties["AbsoluteScalingFactor"] = factor

def SetDirectBeamAbsoluteScale(direct_beam, beamstop_diameter=0.0, attenuator_trans=1.0, apply_sensitivity=False):
    ReductionSingleton().reduction_properties["AbsoluteScaleMethod"] = "ReferenceData"
    ReductionSingleton().reduction_properties["AbsoluteScalingReferenceFilename"] = direct_beam
    ReductionSingleton().reduction_properties["AbsoluteScalingBeamDiameter"] = beamstop_diameter
    ReductionSingleton().reduction_properties["AbsoluteScalingAttenuatorTrans"] = attenuator_trans
    ReductionSingleton().reduction_properties["AbsoluteScalingApplySensitivity"] = apply_sensitivity

def DivideByThickness(thickness=1.0):
    if thickness is None or thickness == 1.0:
        if ReductionSingleton().reduction_properties.has_key("SampleThickness"):
            del ReductionSingleton().reduction_properties["SampleThickness"]
    else:
        ReductionSingleton().reduction_properties["SampleThickness"] = thickness

def Stitch(data_list=[], q_min=None, q_max=None, output_workspace=None,
           scale=None, save_output=False):
    """
        Stitch a set of SANS data sets

        @param data_list: List of workspaces to stitch.
        @param q_min: Minimum Q-value of the overlap between two consecutive data sets.
                      The q_min argument must be an array when stitching more than two data sets.
                      The length of the array should be 1 less than the number of data sets.
        @param q_max: Maximum Q-value of the overlap between two consecutive data sets (must be an array for more than two data sets).
                      The q_max argument must be an array when stitching more than two data sets.
                      The length of the array should be 1 less than the number of data sets.
        @param output_workspace: Name of the output workspace containing the stitched data.
        @param scale: Scaling factor.
                      The scaling factor should either be a single number
                      or a list of length equal to the number of data sets.
                      The former will scale everything by the given factor, while the
                      latter will assign the given scaling factors to the data sets.
        @param save_output: If true, the output will be saved in the current working directory.
    """
    from LargeScaleStructures.data_stitching import stitch
    stitch(data_list, q_min=q_min, q_max=q_max, output_workspace=output_workspace,
           scale=scale, save_output=save_output)
