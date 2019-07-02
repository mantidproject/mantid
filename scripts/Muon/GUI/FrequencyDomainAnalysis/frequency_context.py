# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from re import findall

class MaxEnt(object):

    def __init__(self, run, ws_freq):
        print(run)
        self.run = run
        self.ws_freq = ws_freq


class FFT(object):
    # fft has two runs, one for Re and one for Im

    def __init__(self, ws_freq_name, Re_run, Re, Im_run, Im, phasequad):
        self.Re_run = Re_run
        self.Im_run = Im_run
        self.ws_freq_name = ws_freq_name
        self.Re = Re
        self.Im = Im
        self.phasequad = phasequad


FREQUENCY_EXTENSIONS ={"MOD":"mod", "RE":"Re", "IM":"Im", "MAXENT":"MaxEnt", "FFT":"FFT All" }


class FrequencyContext(object):

    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MonAnalysis
    """

    def __init__(self):
        self._maxEnt_freq = []
        self._FFT_freq = []
        self.plot_type = "None"

    def add_maxEnt(self, run, ws_freq):
        self._maxEnt_freq.append(MaxEnt(run, ws_freq))

    @property
    def maxEnt_freq(self):
        return [maxEnt.ws_freq.name() for maxEnt in self._maxEnt_freq]

    def add_FFT(self, ws_freq_name, Re_run, Re, Im_run, Im, phasequad=False):
        self._FFT_freq.append(
            FFT(ws_freq_name,
                Re_run,
                Re,
                Im_run,
                Im,
                phasequad=phasequad))

    @property
    def FFT_freq(self):
        return [FFT.ws_freq_name for FFT in self._FFT_freq]

    def get_frequency_workspace_names(self, run_list, group, pair, phasequad, frequency_type):
        # do MaxEnt first as it only has run number
        names = []
        for maxEnt in self._maxEnt_freq:
            for runs in run_list:
                for run in runs:
                    if int(run) == int(maxEnt.run):
                        names.append(maxEnt.ws_freq.name())
        # do FFT
        for fft in self._FFT_freq:
            for runs in run_list:
                for run in runs:
                    if (int(run) == int(fft.Re_run) or int(run) == int(fft.Im_run)) and \
                       (fft.Re in group or fft.Re in pair or fft.Im in group or fft.Im in pair):
                        names.append(fft.ws_freq_name)
                    # do phaseQuad - will only have one run
                    elif int(run) == int(fft.Re_run) and phasequad and fft.phasequad:
                        names.append(fft.ws_freq_name)
        if frequency_type == "All":
            return names
        elif frequency_type ==FREQUENCY_EXTENSIONS["FFT"]:
		    return [name for name in names if FREQUENCY_EXTENSIONS["MAXENT"] not in name]
        else:
            output = []
            count = 1
            """ if Re or Im then the count will be 2
            appears as part of the FFT name"""
            if len(frequency_type) == 2:
               count = 2
            for name in names:
                num = len(findall(frequency_type, name))
                if frequency_type in name and count == num:
                   output.append(name)
            return output