# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class RundexSettings(object):

    TECHNIQUE_MAP = {'D11': 'sans', 'D16': 'sans', 'D22': 'sans', 'D33': 'sans',
                     'D17': 'refl', 'FIGARO': 'refl'}

    COLUMNS = {
        'sans': ["SampleRuns", "SampleTransmissionRuns", "AbsorberRuns", "BeamRuns", "FluxRuns", "ContainerRuns",
                 "ContainerTransmissionRuns", "TransmissionAbsorberRuns", "TransmissionBeamRuns", "MaskFiles",
                 "ReferenceFiles", "CustomOptions"],
        'refl': ["DirectBeam", "ReflectedBeam", "AngleOption", "Method"]
    }

    ALGORITHMS = {
        'sans': "SANSILLAutoProcess",
        'refl': "ReflectometryILLAutoProcess"
    }

    @staticmethod
    def get_technique(instrument):
        if instrument in RundexSettings.TECHNIQUE_MAP.keys():
            return RundexSettings.TECHNIQUE_MAP[instrument]
        else:
            raise RuntimeError('Instrument {0} is not yet supported.'.format(instrument))