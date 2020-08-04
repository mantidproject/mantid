# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class RundexSettings(object):

    # instruments
    D11 =    "D11"
    D16 =    "D16"
    D22 =    "D22"
    D33 =    "D33"
    D17 =    "D17"
    FIGARO = "FIGARO"

    # techniques
    SANS = "SANS"
    REFL = "Reflectometry"

    # acquisition modes
    SANS_ACQ =   "SANS"
    REFL_POL =   "Polarized"
    REFL_NPOL =  "Unpolarized"

    # correspondance between instrument and technique
    TECHNIQUE = {
            D11:    SANS,
            D16:    SANS,
            D22:    SANS,
            D33:    SANS,
            D17:    REFL,
            FIGARO: REFL
            }

    # correspondance between instrument and acquisition mode
    ACQUISITION_MODES = {
            D11:    [SANS_ACQ],
            D16:    [SANS_ACQ],
            D22:    [SANS_ACQ],
            D33:    [SANS_ACQ],
            D17:    [REFL_POL, REFL_NPOL],
            FIGARO: [REFL_NPOL]
            }

    # parameters for each acquisition mode
    COLUMNS = {
            SANS_ACQ: [
                "SampleRuns",
                "SampleTransmissionRuns",
                "AbsorberRuns",
                "BeamRuns",
                "FluxRuns",
                "ContainerRuns",
                "ContainerTransmissionRuns",
                "TransmissionAbsorberRuns",
                "TransmissionBeamRuns",
                "MaskFiles",
                "ReferenceFiles",
                "OutputWorkspace",
                "WedgeWorkspace",
                "SampleThickness",
                "CustomOptions"
                ],
            REFL_POL: [
                "Run00",
                "Run01",
                "Run10",
                "Run11",
                "DirectRun",
                "OutputWorkspace",
                "AngleOption",
                "Theta",
                "SummationType",
                "CustomOptions",
                "GlobalScaleFactor",
                "UseManualScaleFactors",
                "ManualScaleFactors"
                ],
            REFL_NPOL: [
                "Run",
                "DirectRun",
                "OutputWorkspace",
                "GlobalScaleFactor",
                "UseManualScaleFactors",
                "ManualScaleFactors",
                "AngleOption",
                "Theta",
                "SummationType",
                "CustomOptions"
                ]
            }

    # algo name for each acquisition mode
    ALGORITHM = {
            SANS_ACQ:   "SANSILLAutoProcess",
            REFL_POL:   "ReflectometryILLAutoProcess",
            REFL_NPOL:  "ReflectometryILLAutoProcess"
            }

    # ideal number of threads for each acquisition mode (optional).
    # If not provided, Qt will decide
    THREADS_NUMBER = {
            }

    # settings for each acquisition mode
    SETTINGS = {
            SANS_ACQ : {
                "ThetaDependent": True,
                "SensitivityMaps": "",
                "DefaultMaskFile": "",
                "NormaliseBy": "Timer",
                "SampleThickness": "0.1",
                "BeamRadius": "0.05",
                "WaterCrossSection": "1",
                "OutputType": "I(Q)",
                "CalculateResolution": "None",
                "DefaultQBinning": "PixelSizeBased",
                "BinningFactor": "1",
                "OutputBinning": "",
                "NPixelDivision": "1",
                "NumberOfWedges": "0",
                "WedgeAngle": "30",
                "WedgeOffset": "0",
                "AsymmetricWedges": False,
                "MaxQxy": "0",
                "DeltaQ": "0",
                "IQxQyLogBinning": False
                },
            REFL_POL : {
                "PolarizationEfficiencyFile": "",
                "DirectFlatBackground": "Background Average",
                "ReflFlatBackground": "Background Constant Fit",
                "SubalgorithmLogging": "Logging OFF",
                "Cleanup": "Cleanup ON",
                "WaterWorkspace": "",
                "SlitNormalisation": "Slit Normalisation AUTO",
                "FluxNormalisation": "Normalise To Time",
                "CacheDirectBeam": False,
                "WavelengthLowerBound": "0",
                "WavelengthUpperBound": "35",
                "DeltaQFractionBinning": "0.5",
                "DirectLowAngleFrgHalfWidth": "0",
                "DirectLowAngleBkgOffset": "7",
                "DirectLowAngleBkgWidth": "5",
                "DirectHighAngleFrgHalfWidth": "0",
                "DirectHighAngleBkgOffset": "7",
                "DirectHighAngleBkgWidth": "5",
                "DirectFitStartWorkspaceIndex": "0",
                "DirectFitEndWorkspaceIndex": "255",
                "DirectFitWavelengthLowerBound": "-1",
                "DirectFitWavelengthUpperBound": "-1",
                "ReflLowAngleFrgHalfWidth": "0",
                "ReflLowAngleBkgOffset": "7",
                "ReflLowAngleBkgWidth": "5",
                "ReflHighAngleFrgHalfWidth": "0",
                "ReflHighAngleBkgOffset": "7",
                "ReflHighAngleBkgWidth": "5",
                "ReflFitStartWorkspaceIndex": "0",
                "ReflFitEndWorkspaceIndex": "255",
                "ReflFitWavelengthLowerBound": "-1",
                "ReflFitWavelengthUpperBound": "-1"
                },
            REFL_NPOL : {
                "DirectFlatBackground": "Background Constant Fit",
                "ReflFlatBackground": "Background Constant Fit",
                "SubalgorithmLogging": "Logging OFF",
                "Cleanup": "Cleanup ON",
                "WaterWorkspace": "",
                "SlitNormalisation": "Slit Normalisation AUTO",
                "FluxNormalisation": "Normalise To Time",
                "CacheDirectBeam": False,
                "WavelengthLowerBound": "0",
                "WavelengthUpperBound": "35",
                "DeltaQFractionBinning": "0.5",
                "DirectLowAngleFrgHalfWidth": "0",
                "DirectLowAngleBkgOffset": "7",
                "DirectLowAngleBkgWidth": "5",
                "DirectHighAngleFrgHalfWidth": "0",
                "DirectHighAngleBkgOffset": "7",
                "DirectHighAngleBkgWidth": "5",
                "DirectFitStartWorkspaceIndex": "0",
                "DirectFitEndWorkspaceIndex": "255",
                "DirectFitWavelengthLowerBound": "-1",
                "DirectFitWavelengthUpperBound": "-1",
                "ReflLowAngleFrgHalfWidth": "0",
                "ReflLowAngleBkgOffset": "7",
                "ReflLowAngleBkgWidth": "5",
                "ReflHighAngleFrgHalfWidth": "0",
                "ReflHighAngleBkgOffset": "7",
                "ReflHighAngleBkgWidth": "5",
                "ReflFitStartWorkspaceIndex": "0",
                "ReflFitEndWorkspaceIndex": "255",
                "ReflFitWavelengthLowerBound": "-1",
                "ReflFitWavelengthUpperBound": "-1"
                }
            }

    # Json keys
    INSTRUMENT_JSON_KEY = "Instrument"
    TECHNIQUE_JSON_KEY = "Technique"
    MODE_JSON_KEY = "AcquisitionMode"
    CYCLE_JSON_KEY = "CycleNumber"
    EXPERIMENT_JSON_KEY = "ExperimentID"
    SETTINGS_JSON_KEY = "GlobalSettings"
    SAMPLES_JSON_KEY = "Samples"
    CUSTOM_OPT_JSON_KEY = "CustomOptions"
    VISUAL_SETTINGS_JSON_KEY = "VisualSettings"
