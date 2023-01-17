# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

ABSOLUTE_UNITS_OFF = "Absolute Units OFF"
ABSOLUTE_UNITS_ON = "Absolute Units ON"

BEAM_STOP_DIAGNOSTICS_AUTO = "Beam Stop Diagnostics AUTO"
BEAM_STOP_DIAGNOSTICS_OFF = "Beam Stop Diagnostics OFF"
BEAM_STOP_DIAGNOSTICS_ON = "Beam Stop Diagnostics ON"

BKG_AUTO = "Flat Bkg AUTO"
BKG_OFF = "Flat Bkg OFF"
BKG_ON = "Flat Bkg ON"

BKG_DIAGNOSTICS_AUTO = "Bkg Diagnostics AUTO"
BKG_DIAGNOSTICS_OFF = "Bkg Diagnostics OFF"
BKG_DIAGNOSTICS_ON = "Bkg Diagnostics ON"

CATEGORIES = "ILL\\Direct;Inelastic\\Reduction;Workflow\\Inelastic"

DEFAULT_MASK_OFF = "Default Mask OFF"
DEFAULT_MASK_ON = "Default Mask ON"

DWF_OFF = "Correction OFF"
DWF_ON = "Correction ON"

ELASTIC_CHANNEL_AUTO = "Elastic Channel AUTO"
ELASTIC_CHANNEL_FIT = "Fit Elastic Channel"
ELASTIC_CHANNEL_SAMPLE_LOG = "Default Elastic Channel"

ELASTIC_PEAK_DIAGNOSTICS_AUTO = "Peak Diagnostics AUTO"
ELASTIC_PEAK_DIAGNOSTICS_OFF = "Peak Diagnostics OFF"
ELASTIC_PEAK_DIAGNOSTICS_ON = "Peak Diagnostics ON"

EPP_METHOD_AUTO = "EPP Method AUTO"
EPP_METHOD_FIT = "Fit EPP"
EPP_METHOD_CALCULATE = "Calculate EPP"

INCIDENT_ENERGY_CALIBRATION_AUTO = "Energy Calibration AUTO"
INCIDENT_ENERGY_CALIBRATION_OFF = "Energy Calibration OFF"
INCIDENT_ENERGY_CALIBRATION_ON = "Energy Calibration ON"

INDEX_TYPE_DET_ID = "Detector ID"
INDEX_TYPE_WS_INDEX = "Workspace Index"
INDEX_TYPE_SPECTRUM_NUMBER = "Spectrum Number"

NORM_METHOD_MON = "Normalisation Monitor"
NORM_METHOD_OFF = "Normalisation OFF"
NORM_METHOD_TIME = "Normalisation Time"

PROP_ABSOLUTE_UNITS = "AbsoluteUnitsNormalisation"
PROP_BEAM_STOP_DIAGNOSTICS = "BeamStopDiagnostics"
PROP_BEAM_STOP_THRESHOLD = "BeamStopThreshold"
PROP_BINNING_PARAMS_Q = "QBinningParams"
PROP_BKG_DIAGNOSTICS = "BkgDiagnostics"
PROP_BKG_DIAGNOSTICS_HIGH_THRESHOLD = "NoisyBkgHighThreshold"
PROP_BKG_DIAGNOSTICS_LOW_THRESHOLD = "NoisyBkgLowThreshold"
PROP_BKG_DIAGNOSTICS_SIGNIFICANCE_TEST = "NoisyBkgErrorThreshold"
PROP_BKG_SIGMA_MULTIPLIER = "NonBkgRegionInSigmas"
PROP_CLEANUP_MODE = "Cleanup"
PROP_DEFAULT_MASK = "DefaultMask"
PROP_DET_GROUPING = "DetectorGrouping"
PROP_DET_HOR_GROUPING = "GroupDetHorizontallyBy"
PROP_DET_VER_GROUPING = "GroupDetVerticallyBy"
PROP_DET_GROUPING_BY = "GroupDetBy"
PROP_DIAGNOSTICS_WS = "DiagnosticsWorkspace"
PROP_DWF_CORRECTION = "DebyeWallerCorrection"
PROP_EC_SCALING = "EmptyContainerScaling"
PROP_EC_WS = "EmptyContainerWorkspace"
PROP_ELASTIC_PEAK_DIAGNOSTICS = "ElasticPeakDiagnostics"
PROP_ELASTIC_CHANNEL_MODE = "ElasticChannel"
PROP_ELASTIC_CHANNEL_WS = "ElasticChannelWorkspace"
PROP_ELASTIC_PEAK_SIGMA_MULTIPLIER = "ElasticPeakWidthInSigmas"
PROP_EPP_METHOD = "EPPCreationMethod"
PROP_EPP_SIGMA = "SigmaForCalculatedEPP"
PROP_EPP_WS = "EPPWorkspace"
PROP_FLAT_BKG = "FlatBkg"
PROP_FLAT_BKG_SCALING = "FlatBkgScaling"
PROP_FLAT_BKG_WINDOW = "FlatBkgAveragingWindow"
PROP_FLAT_BKG_WS = "FlatBkgWorkspace"
PROP_GROUPING_ANGLE_STEP = "GroupingAngleStep"
PROP_INCIDENT_ENERGY_CALIBRATION = "IncidentEnergyCalibration"
PROP_INCIDENT_ENERGY_WS = "IncidentEnergyWorkspace"
PROP_INPUT_FILE = "Run"
PROP_INPUT_WS = "InputWorkspace"
PROP_MON_EPP_WS = "MonitorEPPWorkspace"
PROP_MON_INDEX = "Monitor"
PROP_MON_PEAK_SIGMA_MULTIPLIER = "MonitorPeakWidthInSigmas"
PROP_NORMALISATION = "Normalisation"
PROP_NUMBER_OF_SIMULATION_WAVELENGTHS = "NumberOfSimulatedWavelengths"
PROP_EVENTS_PER_WAVELENGTH = "EventsPerPoint"
PROP_OUTPUT_DET_EPP_WS = "OutputEPPWorkspace"
PROP_OUTPUT_DIAGNOSTICS_REPORT = "OutputReport"
PROP_OUTPUT_DIAGNOSTICS_REPORT_WS = "OutputReportWorkspace"
PROP_OUTPUT_ELASTIC_CHANNEL_WS = "OutputElasticChannelWorkspace"
PROP_OUTPUT_FLAT_BKG_WS = "OutputFlatBkgWorkspace"
PROP_OUTPUT_INCIDENT_ENERGY_WS = "OutputIncidentEnergyWorkspace"
PROP_OUTPUT_MON_EPP_WS = "OutputMonitorEPPWorkspace"
PROP_OUTPUT_RAW_WS = "OutputRawWorkspace"
PROP_OUTPUT_SELF_SHIELDING_CORRECTION_WS = "OutputSelfShieldingCorrectionWorkspace"
PROP_OUTPUT_THETA_W_WS = "OutputSofThetaEnergyWorkspace"
PROP_OUTPUT_WS = "OutputWorkspace"
PROP_PEAK_DIAGNOSTICS_HIGH_THRESHOLD = "ElasticPeakHighThreshold"
PROP_PEAK_DIAGNOSTICS_LOW_THRESHOLD = "ElasticPeakLowThreshold"
PROP_PEAK_DIAGNOSTICS_SIGNIFICANCE_TEST = "ElasticPeakErrorThreshold"
PROP_REBINNING_PARAMS_W = "EnergyRebinningParams"
PROP_REBINNING_W = "EnergyRebinning"
PROP_SELF_SHIELDING_CORRECTION_WS = "SelfShieldingCorrectionWorkspace"
PROP_SIMULATION_INSTRUMENT = "SimulationInstrument"
PROP_SPARSE_INSTRUMENT_COLUMNS = "SparseInstrumentColumns"
PROP_SPARSE_INSTRUMENT_ROWS = "SparseInstrumentRows"
PROP_SUBALG_LOGGING = "SubalgorithmLogging"
PROP_TEMPERATURE = "Temperature"
PROP_TRANSPOSE_SAMPLE_OUTPUT = "Transposing"
PROP_USER_MASK = "MaskedDetectors"
PROP_USER_MASK_COMPONENTS = "MaskedComponents"
PROP_VANA_WS = "IntegratedVanadiumWorkspace"

PROPGROUP_OPTIONAL_OUTPUT = "Optional Output"

SIMULATION_INSTRUMENT_FULL = "Full Instrument"
SIMULATION_INSTRUMEN_SPARSE = "Sparse Instrument"

SIMULATION_SEED = 123456789

SUBALG_LOGGING_OFF = "Logging OFF"
SUBALG_LOGGING_ON = "Logging ON"

TRANSPOSING_OFF = "Transposing OFF"
TRANSPOSING_ON = "Transposing ON"

WS_CONTENT_DETS = 0
WS_CONTENT_MONS = 1


def convertToWorkspaceIndex(i, ws, indexType=INDEX_TYPE_DET_ID):
    """Convert given number to workspace index."""
    if indexType == INDEX_TYPE_WS_INDEX:
        return i
    elif indexType == INDEX_TYPE_SPECTRUM_NUMBER:
        return ws.getIndexFromSpectrumNumber(i)
    else:  # INDEX_TYPE_DET_ID
        for j in range(ws.getNumberHistograms()):
            if ws.getSpectrum(j).hasDetectorID(i):
                return j
        raise RuntimeError("No workspace index found for detector id {0}".format(i))


def convertListToWorkspaceIndices(indices, ws, indexType=INDEX_TYPE_DET_ID):
    """Convert a list of spectrum nubmers/detector IDs to workspace indices."""
    return [convertToWorkspaceIndex(i, ws, indexType) for i in indices]


def get_grouping_pattern(ws, vertical_step, horizontal_step=1):
    """Returns a grouping pattern taking into account the requested grouping, either inside the tube, as defined
    by grouping-by property, or horizontally (between tubes) and vertically (inside a tube).
    The latter method can be applied only to PANTHER, SHARP, and IN5 instruments' structure.

    Keyword parameters:
    ws -- workspace for which the grouping pattern is to be provided
    vertical_step -- grouping step inside a tube
    horizontal_step -- grouping step between tubes
    """
    instrument = ws.getInstrument().getName()
    n_pixels = ws.getNumberHistograms()
    if horizontal_step == 1:
        group_by = vertical_step
        grouping_pattern = ["{}-{}".format(pixel_id, pixel_id + group_by - 1) for pixel_id in range(0, n_pixels - group_by, group_by)]
    else:
        group_by_x = horizontal_step
        group_by_y = vertical_step
        n_tubes = 0
        n_monitors = {"IN5": 1, "PANTHER": 1, "SHARP": 1}
        for comp in ws.componentInfo():
            if len(comp.detectorsInSubtree) > 1 and comp.hasParent:
                n_tubes += 1
        n_tubes -= n_monitors[instrument]
        if instrument == "IN5":  # there is an extra bank that contains all of the tubes
            n_tubes -= 1
        n_pixels_per_tube = int(n_pixels / n_tubes)
        grouping_pattern = []
        pixel_id = 0
        while pixel_id < n_pixels - (group_by_x - 1) * n_pixels_per_tube:
            pattern = []
            for tube_shift in range(0, group_by_x):
                pattern_max = min(n_pixels, pixel_id + tube_shift * n_pixels_per_tube + group_by_y)
                numeric_pattern = list(range(pixel_id + tube_shift * n_pixels_per_tube, pattern_max))
                pattern.append("+".join(map(str, numeric_pattern)))
            pattern = "+".join(pattern)
            grouping_pattern.append(pattern)
            pixel_id += group_by_y
            if pixel_id % n_pixels_per_tube == 0:
                pixel_id += n_pixels_per_tube * (group_by_x - 1)

    return ",".join(grouping_pattern)
