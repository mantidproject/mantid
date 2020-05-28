# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

class RundexSettings(object):

    # techniques names
    SANS = "SANS"
    REFL = "Reflectometry"

    # correspondance between instruments and technique(s)
    TECHNIQUES = {
            'D11':    [SANS],
            'D16':    [SANS],
            'D22':    [SANS],
            'D33':    [SANS],
            'D17':    [REFL],
            'FIGARO': [REFL]
            }

    # parameters for each techniques
    COLUMNS = {
            SANS: [
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
                "CustomOptions"
                ],
            REFL: [
                "DirectBeam",
                "ReflectedBeam",
                "AngleOption",
                "Method"
                ]
            }

    # algo name for each techniques
    ALGORITHMS = {
            SANS: "SANSILLAutoProcess",
            REFL: "ReflectometryILLAutoProcess"
            }

    SETTINGS = {
            SANS: {
                "ThetaDependent": True,
                "SensitivityMaps": None,
                "DefaultMaskFile": None,
                "NormaliseBy": "Timer",
                "SampleThickness": 0.1,
                "BeamRadius": 0.05,
                "WaterCrossSection": 1,
                "OutputType": "I(Q)",
                "CalculateResolution": "None",
                "DefaultQBinning": "PixelSizeBased",
                "BinningFactor": 1,
                "OutputBinning": None,
                "NPixelDivision": 1,
                "NumberOfWedges": 0,
                "WedgeAngle": 30,
                "WedgeOffset": 0,
                "AsymmetricWedges": False,
                "MaxQxy": 0,
                "DeltaQ": 0,
                "IQxQyLogBinning": False,
                "PanelOutputWorkspaces": None
                }
            }

    # Json keys
    INSTRUMENT_JSON_KEY = "Instrument"
    TECHNIQUE_JSON_KEY = "Technique"
    SETTINGS_JSON_KEY = "GlobalSettings"
    SAMPLES_JSON_KEY = "Samples"
    CUSTOM_OPT_JSON_KEY = "CustomOptions"
