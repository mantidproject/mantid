# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import Muon.GUI.Common.ADSHandler.workspace_naming as wsName
from Muon.GUI.Common.contexts.muon_group_pair_context import get_default_grouping
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string


class FrequencyContext(object):
    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MonAnalysis
    """

    def __init__(self):
        self._maxEnt_freq = []
        self._FFT_freq = []

    def add_maxEnt(self, ws_freq):
        self._maxEnt_freq.append(ws_freq)

    @property
    def maxEnt_freq(self):
        return [ws.name() for ws in self._maxEnt_freq]

    def add_FFT(self, ws_freq):
        self._FFT_freq.append(ws_freq)

    @property
    def FFT_freq(self):
        return [ws.name() for ws in self._FFT_freq]
