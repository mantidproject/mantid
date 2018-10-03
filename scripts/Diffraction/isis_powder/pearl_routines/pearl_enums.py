# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class PEARL_FOCUS_MODES(object):
    enum_friendly_name = "focus modes"
    all = "all"
    groups = "groups"
    trans = "trans"
    mods = "mods"


class PEARL_TT_MODES(object):
    enum_friendly_name = "tt_modes"
    all = "all"
    tt_35 = "tt35"
    tt_70 = "tt70"
    tt_88 = "tt88"
