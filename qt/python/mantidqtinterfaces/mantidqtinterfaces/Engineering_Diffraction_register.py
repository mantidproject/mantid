# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.project.decoderfactory import DecoderFactory
from mantidqt.project.encoderfactory import EncoderFactory
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.engineering_diffraction import EngineeringDiffractionGui
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.engineering_diffraction_io import EngineeringDiffractionEncoder, \
    EngineeringDiffractionDecoder


def compatible_check_for_encoder(obj, _):
    return isinstance(obj, EngineeringDiffractionGui)


DecoderFactory.register_decoder(EngineeringDiffractionDecoder)
EncoderFactory.register_encoder(EngineeringDiffractionEncoder, compatible_check_for_encoder)
