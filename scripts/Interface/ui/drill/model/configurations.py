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
                "GlobalScaleFactor",
                "UseManualScaleFactors",
                "ManualScaleFactors",
                "CustomOptions"
                ],
            REFL_NPOL: [
                "Run",
                "DirectRun",
                "OutputWorkspace",
                "AngleOption",
                "Theta",
                "SummationType",
                "GlobalScaleFactor",
                "UseManualScaleFactors",
                "ManualScaleFactors",
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
            SANS_ACQ:  1,
            REFL_POL:  1,
            REFL_NPOL: 1
            }

    # settings for each acquisition mode
    SETTINGS = {
            SANS_ACQ : [
                "ThetaDependent",
                "SensitivityMaps",
                "DefaultMaskFile",
                "NormaliseBy",
                "SampleThickness",
                "BeamRadius",
                "TransmissionBeamRadius",
                "WaterCrossSection",
                "OutputType",
                "CalculateResolution",
                "DefaultQBinning",
                "BinningFactor",
                "OutputBinning",
                "NPixelDivision",
                "NumberOfWedges",
                "WedgeAngle",
                "WedgeOffset",
                "AsymmetricWedges",
                "MaxQxy",
                "DeltaQ",
                "IQxQyLogBinning",
                "OutputPanels"
                ],
            REFL_POL : [
                "PolarizationEfficiencyFile",
                "DirectFlatBackground",
                "ReflFlatBackground",
                "SubalgorithmLogging",
                "Cleanup",
                "WaterWorkspace",
                "SlitNormalisation",
                "FluxNormalisation",
                "CacheDirectBeam",
                "WavelengthLowerBound",
                "WavelengthUpperBound",
                "DeltaQFractionBinning",
                "DirectLowAngleFrgHalfWidth",
                "DirectLowAngleBkgOffset",
                "DirectLowAngleBkgWidth",
                "DirectHighAngleFrgHalfWidth",
                "DirectHighAngleBkgOffset",
                "DirectHighAngleBkgWidth",
                "DirectFitStartWorkspaceIndex",
                "DirectFitEndWorkspaceIndex",
                "DirectFitWavelengthLowerBound",
                "DirectFitWavelengthUpperBound",
                "ReflLowAngleFrgHalfWidth",
                "ReflLowAngleBkgOffset",
                "ReflLowAngleBkgWidth",
                "ReflHighAngleFrgHalfWidth",
                "ReflHighAngleBkgOffset",
                "ReflHighAngleBkgWidth",
                "ReflFitStartWorkspaceIndex",
                "ReflFitEndWorkspaceIndex",
                "ReflFitWavelengthLowerBound",
                "ReflFitWavelengthUpperBound"
                ],
            REFL_NPOL : [
                "DirectFlatBackground",
                "ReflFlatBackground",
                "SubalgorithmLogging",
                "Cleanup",
                "WaterWorkspace",
                "SlitNormalisation",
                "FluxNormalisation",
                "CacheDirectBeam",
                "WavelengthLowerBound",
                "WavelengthUpperBound",
                "DeltaQFractionBinning",
                "DirectLowAngleFrgHalfWidth",
                "DirectLowAngleBkgOffset",
                "DirectLowAngleBkgWidth",
                "DirectHighAngleFrgHalfWidth",
                "DirectHighAngleBkgOffset",
                "DirectHighAngleBkgWidth",
                "DirectFitStartWorkspaceIndex",
                "DirectFitEndWorkspaceIndex",
                "DirectFitWavelengthLowerBound",
                "DirectFitWavelengthUpperBound",
                "ReflLowAngleFrgHalfWidth",
                "ReflLowAngleBkgOffset",
                "ReflLowAngleBkgWidth",
                "ReflHighAngleFrgHalfWidth",
                "ReflHighAngleBkgOffset",
                "ReflHighAngleBkgWidth",
                "ReflFitStartWorkspaceIndex",
                "ReflFitEndWorkspaceIndex",
                "ReflFitWavelengthLowerBound",
                "ReflFitWavelengthUpperBound"
                ]
            }

    # optionnal flags
    FLAGS = {
            REFL_POL : {
                "PolarizationOption": "Polarized"
                },
            REFL_NPOL : {
                "PolarizationOption": "NonPolarized"
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
