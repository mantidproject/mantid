# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class HRPD_TOF_WINDOWS(object):
    enum_friendly_name = "TOF windows"
    window_10_50 = "10-50"
    window_10_110 = "10-110"
    window_30_130 = "30-130"
    window_100_200 = "100-200"
    window_180_280 = "180-280"


class HRPD_MODES(object):
    enum_friendly_name = "Grouping mode"
    coupled = "coupled"
    decoupled = "decoupled"
