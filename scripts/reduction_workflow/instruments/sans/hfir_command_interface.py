# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,unused-import,unused-argument,ungrouped-imports
"""

List of common user commands for HFIR SANS

"""

import os.path
import mantid

from reduction_workflow.command_interface import ReductionSingleton, Clear
from reduction_workflow.find_data import find_data
from reduction_workflow.instruments.sans import hfir_instrument

from mantid.kernel import Logger
from mantid.simpleapi import Load

# The following imports allow users to import this file and have all functionality automatically imported
# Do not remove these imports as it will break user scripts which rely on them
from reduction_workflow.command_interface import OutputPath, Reduce1D, Reduce, AppendDataFile, ClearDataFiles  # noqa: F401


def BIOSANS():
    Clear()
    ReductionSingleton().set_instrument("BIOSANS", "SetupHFIRReduction", "HFIRSANSReduction")
    TimeNormalization()
    SolidAngle()
    AzimuthalAverage()


def GPSANS():
    Clear()
    ReductionSingleton().set_instrument("GPSANS", "SetupHFIRReduction", "HFIRSANSReduction")
    TimeNormalization()
    SolidAngle()
    AzimuthalAverage()


def DataPath(path):
    ReductionSingleton().set_data_path(path)
    ReductionSingleton().set_output_path(path)
    ReductionSingleton().reduction_properties["OutputDirectory"] = path


def DirectBeamCenter(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BeamCenterMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["BeamCenterFile"] = datafile


def ScatteringBeamCenter(datafile, beam_radius=3.0):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["BeamCenterMethod"] = "Scattering"
    ReductionSingleton().reduction_properties["BeamRadius"] = beam_radius
    ReductionSingleton().reduction_properties["BeamCenterFile"] = datafile


def SetBeamCenter(x, y):
    ReductionSingleton().reduction_properties["BeamCenterMethod"] = "Value"
    ReductionSingleton().reduction_properties["BeamCenterX"] = x
    ReductionSingleton().reduction_properties["BeamCenterY"] = y


def TimeNormalization():
    ReductionSingleton().reduction_properties["Normalisation"] = "Timer"


def MonitorNormalization():
    ReductionSingleton().reduction_properties["Normalisation"] = "Monitor"


def NoNormalization():
    ReductionSingleton().reduction_properties["Normalisation"] = "None"


def SensitivityCorrection(flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, use_sample_dc=False):
    flood_data = find_data(flood_data, instrument=ReductionSingleton().get_instrument())
    if dark_current is not None:
        dark_current = find_data(dark_current, instrument=ReductionSingleton().get_instrument())

    ReductionSingleton().reduction_properties["SensitivityFile"] = flood_data
    ReductionSingleton().reduction_properties["MinEfficiency"] = min_sensitivity
    ReductionSingleton().reduction_properties["MaxEfficiency"] = max_sensitivity
    if dark_current is not None:
        ReductionSingleton().reduction_properties["SensitivityDarkCurrentFile"] = dark_current
    elif "SensitivityDarkCurrentFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["SensitivityDarkCurrentFile"]
    if "SensitivityBeamCenterX" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["SensitivityBeamCenterX"]
    if "SensitivityBeamCenterY" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["SensitivityBeamCenterY"]
    ReductionSingleton().reduction_properties["UseDefaultDC"] = use_sample_dc


def SetSensitivityBeamCenter(x, y):
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"] = "Value"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterX"] = x
    ReductionSingleton().reduction_properties["SensitivityBeamCenterY"] = y


def SensitivityDirectBeamCenter(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"] = "DirectBeam"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterFile"] = datafile


def SensitivityScatteringBeamCenter(datafile, beam_radius=3.0):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["SensitivityBeamCenterMethod"] = "Scattering"
    ReductionSingleton().reduction_properties["SensitivityBeamCenterRadius"] = beam_radius
    ReductionSingleton().reduction_properties["SensitivityBeamCenterFile"] = datafile


def NoSensitivityCorrection():
    if "SensitivityFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["SensitivityFile"]


def DarkCurrent(datafile):
    datafile = find_data(datafile, instrument=ReductionSingleton().get_instrument())
    ReductionSingleton().reduction_properties["DarkCurrentFile"] = datafile


def NoDarkCurrent():
    if "DarkCurrentFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["DarkCurrentFile"]


def SolidAngle(detector_tubes=False, detector_wing=False):
    ReductionSingleton().reduction_properties["SolidAngleCorrection"] = True
    ReductionSingleton().reduction_properties["DetectorTubes"] = detector_tubes
    ReductionSingleton().reduction_properties["DetectorWing"] = detector_wing


def NoSolidAngle():
    ReductionSingleton().reduction_properties["SolidAngleCorrection"] = False


def AzimuthalAverage(
    binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False, align_log_with_decades=False
):
    # Suffix is no longer used but kept for backward compatibility
    ReductionSingleton().reduction_properties["DoAzimuthalAverage"] = True
    if binning is not None:
        ReductionSingleton().reduction_properties["IQBinning"] = binning
    elif "IQBinning" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["IQBinning"]
    ReductionSingleton().reduction_properties["IQNumberOfBins"] = n_bins
    ReductionSingleton().reduction_properties["IQLogBinning"] = log_binning
    ReductionSingleton().reduction_properties["NumberOfSubpixels"] = n_subpix
    ReductionSingleton().reduction_properties["ErrorWeighting"] = error_weighting
    ReductionSingleton().reduction_properties["IQAlignLogWithDecades"] = align_log_with_decades


def NoTransmission():
    if "TransmissionValue" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionValue"]
    if "TransmissionError" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionError"]
    if "TransmissionMethod" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionMethod"]
    if "TransmissionBeamRadius" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionBeamRadius"]
    if "TransmissionSampleDataFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionSampleDataFile"]
    if "TransmissionEmptyDataFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionEmptyDataFile"]
    if "ThetaDependentTransmission" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["ThetaDependentTransmission"]


def SetTransmission(trans, error, theta_dependent=True):
    ReductionSingleton().reduction_properties["TransmissionMethod"] = "Value"
    ReductionSingleton().reduction_properties["TransmissionValue"] = trans
    ReductionSingleton().reduction_properties["TransmissionError"] = error
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependent


def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0, theta_dependent=True, use_sample_dc=True):
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
    elif "TransmissionDarkCurrentFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["TransmissionDarkCurrentFile"]


def ThetaDependentTransmission(theta_dependence=True):
    ReductionSingleton().reduction_properties["ThetaDependentTransmission"] = theta_dependence


def BeamSpreaderTransmission(
    sample_spreader,
    direct_spreader,
    sample_scattering,
    direct_scattering,
    spreader_transmission=1.0,
    spreader_transmission_err=0.0,
    theta_dependent=True,
):
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
    if isinstance(datafile, list):
        datafile = ",".join(datafile)
    find_data(datafile, instrument=ReductionSingleton().get_instrument(), allow_multiple=True)
    ReductionSingleton().reduction_properties["BackgroundFiles"] = datafile


def NoBackground():
    ReductionSingleton().reduction_properties["BackgroundFiles"] = ""


def NoBckTransmission():
    if "BckTransmissionValue" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["BckTransmissionValue"]
    if "BckTransmissionError" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["BckTransmissionError"]
    if "BckTransmissionMethod" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["BckTransmissionMethod"]
    if "BckTransmissionBeamRadius" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["BckTransmissionBeamRadius"]
    if "BckTransmissionSampleDataFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["BckTransmissionSampleDataFile"]
    if "BckTransmissionEmptyDataFile" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["BckTransmissionEmptyDataFile"]
    if "BckThetaDependentTransmission" in ReductionSingleton().reduction_properties:
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


def BckBeamSpreaderTransmission(
    sample_spreader,
    direct_spreader,
    sample_scattering,
    direct_scattering,
    spreader_transmission=1.0,
    spreader_transmission_err=0.0,
    theta_dependent=True,
):
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
    ReductionSingleton().reduction_properties["BckTransmissionBeamCenterFile"] = datafile


def BckTransmissionDarkCurrent(dark_current=None):
    if dark_current is not None:
        dark_current = find_data(dark_current, instrument=ReductionSingleton().get_instrument())
        ReductionSingleton().reduction_properties["BckTransmissionDarkCurrentFile"] = dark_current
    elif "BckTransmissionDarkCurrentFile" in ReductionSingleton().reduction_properties:
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
    """Resets the wavelength to the data file default"""
    if "Wavelength" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["Wavelength"]
    if "WavelengthSpread" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["WavelengthSpread"]


def SaveIqAscii(reducer=None, process=""):
    """Old command for backward compatibility"""
    msg = "SaveIqAscii is not longer used:\n  "
    msg += "Please use 'SaveIq' instead\n  "
    Logger("CommandInterface").warning(msg)
    ReductionSingleton().reduction_properties["ProcessInfo"] = str(process)


def SaveIq(output_dir=None, process=""):
    if output_dir is not None:
        ReductionSingleton().reduction_properties["OutputDirectory"] = output_dir
    ReductionSingleton().reduction_properties["ProcessInfo"] = process


def NoSaveIq():
    if "ProcessInfo" in ReductionSingleton().reduction_properties:
        del ReductionSingleton().reduction_properties["ProcessInfo"]


def IQxQy(nbins=100, log_binning=False):
    ReductionSingleton().reduction_properties["Do2DReduction"] = True
    ReductionSingleton().reduction_properties["IQ2DNumberOfBins"] = nbins
    ReductionSingleton().reduction_properties["IQxQyLogBinning"] = log_binning


def NoIQxQy():
    ReductionSingleton().reduction_properties["Do2DReduction"] = False


def Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0, component_name=""):
    """
    Mask edges of a component_name
    By default is the main detector for both GPSANS and BioSans
    """
    ReductionSingleton().reduction_properties["MaskedEdges"] = [nx_low, nx_high, ny_low, ny_high]
    ReductionSingleton().reduction_properties["MaskedComponent"] = component_name


def MaskComponent(component_name):
    """
    Masks a full component by name
    """
    ReductionSingleton().reduction_properties["MaskedFullComponent"] = component_name


def MaskRectangle(x_min, x_max, y_min, y_max):
    masked_pixels = []
    for ix in range(x_min, x_max + 1):
        for iy in range(y_min, y_max + 1):
            masked_pixels.append([ix, iy])
    det_list = hfir_instrument.get_detector_from_pixel(masked_pixels)
    MaskDetectors(det_list)


def MaskDetectors(det_list):
    if "MaskedDetectorList" in ReductionSingleton().reduction_properties:
        ReductionSingleton().reduction_properties["MaskedDetectorList"].extend(det_list)
    else:
        ReductionSingleton().reduction_properties["MaskedDetectorList"] = det_list


def MaskDetectorSide(side_to_mask=None):
    if side_to_mask is None:
        if "MaskedSide" in ReductionSingleton().reduction_properties:
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
        if "SampleThickness" in ReductionSingleton().reduction_properties:
            del ReductionSingleton().reduction_properties["SampleThickness"]
    else:
        ReductionSingleton().reduction_properties["SampleThickness"] = thickness


def SetWedges(number_of_wedges=2, wedge_angle=30.0, wedge_offset=0.0):
    """
    Set the wedge properties
    @param number_of_wedges: number of wedges to calculate
    @param wedge_angle: augular opening of each wedge, in degrees
    @param wedge_offset: angular offset for the wedges, in degrees
    """
    ReductionSingleton().reduction_properties["NumberOfWedges"] = number_of_wedges
    ReductionSingleton().reduction_properties["WedgeAngle"] = wedge_angle
    ReductionSingleton().reduction_properties["WedgeOffset"] = wedge_offset


def Stitch(data_list=None, q_min=None, q_max=None, output_workspace=None, scale=None, save_output=False):
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
    if data_list is None:
        data_list = []
    from LargeScaleStructures.data_stitching import stitch

    stitch(data_list, q_min=q_min, q_max=q_max, output_workspace=output_workspace, scale=scale, save_output=save_output)


def beam_center_gravitational_drop(beam_center_file, sdd=1.13):
    """
    This method is used for correcting for gravitational drop
    @param beam_center_file :: file where the beam center was found
    @param sdd :: sample detector distance to apply the beam center
    """

    def calculate_neutron_drop(path_length, wavelength):
        """
        Calculate the gravitational drop of the neutrons
        path_length in meters
        wavelength in Angstrom
        """
        wavelength *= 1e-10
        neutron_mass = 1.674927211e-27
        gravity = 9.80665
        h_planck = 6.62606896e-34
        l_2 = (gravity * neutron_mass**2 / (2.0 * h_planck**2)) * path_length**2
        return wavelength**2 * l_2

    # Get beam center used in the previous reduction
    pm = mantid.PropertyManagerDataService[ReductionSingleton().property_manager]
    beam_center_x = pm["LatestBeamCenterX"].value
    beam_center_y = pm["LatestBeamCenterY"].value
    Logger("CommandInterface").information("Beam Center before: [%.2f, %.2f] pixels" % (beam_center_x, beam_center_y))

    try:
        # check if the workspace still exists
        wsname = "__beam_finder_" + os.path.splitext(beam_center_file)[0]
        ws = mantid.mtd[wsname]
        Logger("CommandInterface").debug("Using Workspace: %s." % (wsname))
    except KeyError:
        # Let's try loading the file. For some reason the beamcenter ws is not there...
        try:
            ws = Load(beam_center_file)
            Logger("CommandInterface").debug("Using filename %s." % (beam_center_file))
        except IOError:
            Logger("CommandInterface").error("Cannot read input file %s." % beam_center_file)
            return

    i = ws.getInstrument()
    y_pixel_size_mm = i.getNumberParameter("y-pixel-size")[0]
    Logger("CommandInterface").debug("Y Pixel size = %.2f mm" % y_pixel_size_mm)
    y_pixel_size = y_pixel_size_mm * 1e-3  # In meters
    distance_detector1 = i.getComponentByName("detector1").getPos()[2]
    path_length = distance_detector1 - sdd
    Logger("CommandInterface").debug("SDD detector1 = %.3f meters. SDD for wing = %.3f meters." % (distance_detector1, sdd))
    Logger("CommandInterface").debug("Path length for gravitational drop = %.3f meters." % (path_length))
    r = ws.run()
    wavelength = r.getProperty("wavelength").value
    Logger("CommandInterface").debug("Wavelength = %.2f A." % (wavelength))

    drop = calculate_neutron_drop(path_length, wavelength)
    Logger("CommandInterface").debug("Gravitational drop = %.6f meters." % (drop))
    # 1 pixel -> y_pixel_size
    # x pixel -> drop
    drop_in_pixels = drop / y_pixel_size
    new_beam_center_y = beam_center_y + drop_in_pixels
    Logger("CommandInterface").information("Beam Center after:   [%.2f, %.2f] pixels" % (beam_center_x, new_beam_center_y))
    return beam_center_x, new_beam_center_y
