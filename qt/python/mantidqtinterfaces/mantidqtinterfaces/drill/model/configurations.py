# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class RundexSettings(object):
    # instruments
    D11 = "D11"
    D16 = "D16"
    D22 = "D22"
    D33 = "D33"
    D17 = "D17"
    FIGARO = "FIGARO"
    D2B = "D2B"
    D20 = "D20"
    D1B = "D1B"
    IN1 = "IN1"
    IN4 = "IN4"
    IN5 = "IN5"
    IN6 = "IN6"
    PANTHER = "PANTHER"
    SHARP = "SHARP"

    # techniques (see instrument/Facilities.xml)
    SANS = "SANS"
    REFL = "Reflectometry"
    POWDER = "Powder diffraction"
    DIRECT = "Direct geometry"
    SPECTRO = "Neutron Spectroscopy"

    # acquisition modes
    SANS_ACQ = "SANS v1"
    SANS_PSCAN = "Sample scan"
    SANS_MULTI = "SANS v2"
    REFL_POL = "Polarized"
    REFL_NPOL = "Unpolarized"
    POWDER_DSCAN = "Detector scan"
    POWDER_PSCAN = "Sample scan"
    DIRECT_TOF = "TOF"
    SPECTRO_PSCAN = "Sample scan"

    # correspondance between instrument and technique
    TECHNIQUE = {
        D11: SANS,
        D16: SANS,
        D22: SANS,
        D33: SANS,
        D17: REFL,
        FIGARO: REFL,
        D2B: POWDER,
        D20: POWDER,
        D1B: POWDER,
        IN1: SPECTRO,
        IN4: DIRECT,
        IN5: DIRECT,
        IN6: DIRECT,
        PANTHER: DIRECT,
        SHARP: DIRECT,
    }

    # correspondance between instrument and acquisition mode
    ACQUISITION_MODES = {
        D11: [SANS_ACQ, SANS_MULTI],
        D16: [SANS_ACQ, SANS_PSCAN],
        D22: [SANS_ACQ, SANS_MULTI],
        D33: [SANS_ACQ, SANS_MULTI],
        D17: [REFL_POL, REFL_NPOL],
        FIGARO: [REFL_NPOL],
        D2B: [POWDER_DSCAN],
        D20: [POWDER_DSCAN, POWDER_PSCAN],
        D1B: [POWDER_PSCAN],
        IN1: [SPECTRO_PSCAN],
        IN4: [DIRECT_TOF],
        IN5: [DIRECT_TOF],
        IN6: [DIRECT_TOF],
        PANTHER: [DIRECT_TOF],
        SHARP: [DIRECT_TOF],
    }

    # group by group processing mode
    COLUMN_BY_COLUMN = "ColumnByColumn"
    GROUP_BY_GROUP = "GroupByGroup"
    PROCESSING_MODE = {SANS_MULTI: GROUP_BY_GROUP}
    GROUPED_COLUMNS = {
        SANS_MULTI: [
            "SampleRunsD1",
            "SampleRunsD2",
            "SampleRunsD3",
            "SampleRunsD4",
            "SampleRunsD5",
            "DarkCurrentRuns",
            "EmptyBeamRuns",
            "FluxRuns",
            "EmptyContainerRuns",
            "SampleTrRunsW1",
            "SampleTrRunsW2",
            "TrDarkCurrentRuns",
            "ContainerTrRuns",
            "TrEmptyBeamRuns",
            "BeamStopMasks",
            "FlatFields",
            "Solvents",
            "SampleThickness",
            "SampleNames",
        ]
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
            "SolventFiles",
            "OutputWorkspace",
            "SampleThickness",
            "CustomOptions",
        ],
        SANS_PSCAN: ["SampleRuns", "AbsorberRuns", "ContainerRuns", "OutputWorkspace", "OutputJoinedWorkspace", "CustomOptions"],
        SANS_MULTI: [
            "SampleRunsD1",
            "SampleRunsD2",
            "SampleRunsD3",
            "SampleRunsD4",
            "SampleRunsD5",
            "DarkCurrentRuns",
            "EmptyBeamRuns",
            "FluxRuns",
            "EmptyContainerRuns",
            "SampleTrRunsW1",
            "SampleTrRunsW2",
            "TrDarkCurrentRuns",
            "ContainerTrRuns",
            "TrEmptyBeamRuns",
            "BeamStopMasks",
            "FlatFields",
            "Solvents",
            "SampleThickness",
            "SampleNames",
            "OutputWorkspace",
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
            "CustomOptions",
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
            "CustomOptions",
        ],
        POWDER_DSCAN: ["Run", "OutputWorkspace", "CustomOptions"],
        POWDER_PSCAN: ["Run", "OutputWorkspace", "CustomOptions"],
        DIRECT_TOF: [
            "Runs",
            "OutputWorkspace",
            "ProcessAs",
            "CadmiumWorkspace",
            "EmptyContainerWorkspace",
            "FlatBackground",
            "VanadiumWorkspace",
            "MaskWorkspace",
            "SampleMaterial",
            "SampleGeometry",
            "CustomOptions",
        ],
        SPECTRO_PSCAN: ["SampleRuns", "OutputWorkspace", "ContainerRuns", "CorrectionFile"],
    }

    VISUAL_SETTINGS = {
        SANS_ACQ: {"HiddenColumns": ["FluxRuns", "TransmissionAbsorberRuns"]},
        DIRECT_TOF: {
            "HiddenColumns": [
                "CadmiumWorkspace",
                "FlatBackground",
                "SampleMaterial",
                "SampleGeometry",
            ]
        },
    }

    # algo name for each acquisition mode
    ALGORITHM = {
        SANS_ACQ: "SANSILLAutoProcess",
        SANS_PSCAN: "SANSILLParameterScan",
        SANS_MULTI: "SANSILLMultiProcess",
        REFL_POL: "ReflectometryILLAutoProcess",
        REFL_NPOL: "ReflectometryILLAutoProcess",
        POWDER_DSCAN: "PowderILLDetectorScan",
        POWDER_PSCAN: "PowderILLParameterScan",
        DIRECT_TOF: "DirectILLAutoProcess",
        SPECTRO_PSCAN: "LagrangeILLReduction",
    }

    # export algos for each acquisition mode. Each algo has a boolean to set
    # it as activated or not
    EXPORT_ALGORITHMS = {
        SANS_ACQ: {
            ("SaveNexusProcessed", ".nxs"): False,
            ("SaveAscii", ".txt"): False,
            ("SaveCanSAS1D", ".xml"): True,
            ("SaveNISTDAT", ".dat"): True,
        },
        SANS_MULTI: {
            ("SaveNexusProcessed", ".nxs"): False,
            ("SaveAscii", ".txt"): False,
            ("SaveCanSAS1D", "xml"): True,
            ("SaveNISTDAT", ".dat"): True,
        },
        REFL_POL: {
            ("SaveNexusProcessed", ".nxs"): False,
            ("SaveAscii", ".txt"): False,
            ("SaveReflectometryAscii", ".mft"): True,
            ("SaveReflectometryAscii", ".lam"): False,
        },
        REFL_NPOL: {
            ("SaveNexusProcessed", ".nxs"): False,
            ("SaveAscii", ".txt"): False,
            ("SaveReflectometryAscii", ".mft"): True,
            ("SaveReflectometryAscii", ".lam"): False,
        },
        POWDER_DSCAN: {("SaveNexusProcessed", ".nxs"): False, ("SaveAscii", ".txt"): False, ("SaveFocusedXYE", ".dat"): True},
        POWDER_PSCAN: {("SaveNexusProcessed", ".nxs"): False, ("SaveAscii", ".txt"): False, ("SaveFocussedXYE", ".dat"): True},
        DIRECT_TOF: {("SaveNexusProcessed", ".nxs"): True, ("SaveAscii", ".txt"): False, ("SaveNXSPE", ".nxspe"): False},
    }

    EXPORT_ALGO_CRITERIA = {
        "SaveCanSAS1D": "%OutputType% == 'I(Q)'",
        "SaveNISTDAT": "%OutputType% == 'I(Qx,Qy)'",
    }

    # ideal number of threads for each acquisition mode (optional)
    # if not provided, Qt will decide, which will likely be the number of cores
    # for the moment, limit those to 1 until the algorithms are made truly thread safe
    THREADS_NUMBER = {
        SANS_ACQ: 1,
        SANS_PSCAN: 1,
        SANS_MULTI: 1,
        REFL_POL: 1,
        REFL_NPOL: 1,
        POWDER_DSCAN: 1,
        POWDER_PSCAN: 1,
        DIRECT_TOF: 1,
    }

    # settings for each acquisition mode

    # optionnal flags
    FLAGS = {REFL_POL: {"PolarizationOption": "Polarized"}, REFL_NPOL: {"PolarizationOption": "NonPolarized"}}
    SETTINGS = {
        SANS_ACQ: [
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
            "OutputPanels",
            "WavelengthRange",
            "StitchReferenceIndex",
            "ClearCorrected2DWorkspace",
            "ShapeTable",
            "Wavelength",
        ],
        SANS_PSCAN: ["SensitivityMap", "DefaultMaskFile", "NormaliseBy", "Observable", "PixelYMin", "PixelYMax", "Wavelength"],
        SANS_MULTI: [
            "SensitivityMap",
            "DefaultMask",
            "TransmissionThetaDependent",
            "NormaliseBy",
            "TrBeamRadius",
            "BeamRadius",
            "SampleThicknessFrom",
            "SampleNamesFrom",
            "WaterCrossSection",
            "ProduceSensitivity",
            "SensitivityWithOffset",
            "OutputType",
            "DistancesAtWavelength2",
            "OutputBinning",
            "CalculateResolution",
            "DefaultQBinning",
            "BinningFactor",
            "NumberOfWedges",
            "WedgeAngle",
            "WedgeOffset",
            "AsymmetricWedges",
            "WavelengthRange",
            "ShapeTable",
            "OutputPanels",
            "PerformStitching",
            "ManualScaleFactors",
            "TieScaleFactors",
            "ScaleFactorCalculation",
            "StitchReferenceIndex",
        ],
        REFL_POL: [
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
            "ReflFitWavelengthUpperBound",
            "CorrectGravity",
        ],
        REFL_NPOL: [
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
            "ReflFitWavelengthUpperBound",
            "CorrectGravity",
        ],
        POWDER_DSCAN: [
            "NormaliseTo",
            "CalibrationFile",
            "UseCalibratedData",
            "Output2DTubes",
            "Output2D",
            "Output1D",
            "CropNegativeScatteringAngles",
            "HeightRange",
            "InitialMask",
            "FinalMask",
            "ComponentsToMask",
            "ComponentsToReduce",
            "AlignTubes",
        ],
        POWDER_PSCAN: [
            "CalibrationFile",
            "ROCCorrectionFile",
            "NormaliseTo",
            "ROI",
            "Observable",
            "SortObservableAxis",
            "ScanAxisBinWidth",
            "CropNegative2Theta",
            "ZeroCountingCells",
            "Unit",
        ],
        DIRECT_TOF: [
            "ReductionType",
            "FlatBkg",
            "FlatBkgAveragingWindow",
            "FlatBkgScaling",
            "EmptyContainerScaling",
            "IncidentEnergyCalibration",
            "IncidentEnergy",
            "IncidentEnergyRange",
            "ElasticChannel",
            "EPPCreationMethod",
            "ElasticChannelIndex",
            "EPPWorkspace",
            "AbsorptionCorrection",
            "SelfAttenuationMethod",
            "ContainerMaterial",
            "ContainerGeometry",
            "AbsoluteUnitNormalisation",
            "MaskedTubes",
            "MaskThresholdMin",
            "MaskThresholdMax",
            "MaskedAngles",
            "MaskWithVanadium",
            "EnergyExchangeBinning",
            "MomentumTransferBinning",
            "DetectorGrouping",
            "GroupPixelsBy",
            "GroupingAngleStep",
            "GroupingBehaviour",
            "SampleAngleOffset",
            "SaveOutput",
            "ClearCache",
        ],
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
    EXPORT_JSON_KEY = "ExportAlgorithms"
