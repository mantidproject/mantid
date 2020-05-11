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

