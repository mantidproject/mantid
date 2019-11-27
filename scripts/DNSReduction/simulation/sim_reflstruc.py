# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Reflection data structure for DNS simulation
"""
from __future__ import (absolute_import, division, print_function)


class SimReflStruc(object):
    """
    Reflection data structure for DNS simulation
    """
    def __init__(self):
        super(SimReflStruc, self).__init__()
        self.hkl = None
        self.unique = None
        self.q = None
        self.fs = None
        self.d = None
        self.tth = None
        self.fs_lc = None
        self.fs = None
        self.h = None
        self.k = None
        self.l = None
        self.equivalents = None
        self.M = None
        self.diff = None
        self.det_rot = None
        self.inplane = None
        self.channel = None
        self.det_rot = None
        self.omega = None
        self.sample_rot = None
