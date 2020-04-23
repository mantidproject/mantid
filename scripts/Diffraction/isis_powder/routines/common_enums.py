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
