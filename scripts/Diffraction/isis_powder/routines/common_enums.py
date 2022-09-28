# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Holds enumeration classes for common values


class INPUT_BATCHING(object):
    enum_friendly_name = "input batching mode"
    Individual = "Individual"
    Summed = "Summed"


class WORKSPACE_UNITS(object):
    enum_friendly_name = "workspace units"
    d_spacing = "dSpacing"
    tof = "TOF"
    wavelength = "Wavelength"


class EMPTY_CAN_SUBTRACTION_METHOD(object):
    enum_friendly_name = "empty can subtraction method"
    simple = "Simple"  # default
    paalman_pings = "PaalmanPings"


class VAN_NORMALISATION_METHOD(object):
    enum_friendly_name = "vanadium normalisation method"
    relative = "Relative"  # default
    absolute = "Absolute"
