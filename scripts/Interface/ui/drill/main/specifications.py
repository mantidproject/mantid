# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class RundexSettings(object):

    TECHNIQUES = {
            'D11':    ['SANS'],
            'D16':    ['SANS'],
            'D22':    ['SANS'],
            'D33':    ['SANS'],
            'D17':    ['Reflectometry'],
            'FIGARO': ['Reflectometry']
            }

    COLUMNS = {
            'SANS': [
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
                "CustomOptions"
                ],
            'Reflectometry': [
                "DirectBeam",
                "ReflectedBeam",
                "AngleOption",
                "Method"
                ]
            }

    ALGORITHMS = {
            'SANS':          "SANSILLAutoProcess",
            'Reflectometry': "ReflectometryILLAutoProcess"
            }

