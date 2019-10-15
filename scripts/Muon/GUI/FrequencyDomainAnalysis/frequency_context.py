# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from re import findall
from six import iteritems
from mantid import AnalysisDataService


class MaxEnt(object):

    def __init__(self, run, ws_freq):
        self.run = run
        self.ws_freq = ws_freq


class FFT(object):
    # fft has two runs, one for Re and one for Im

    def __init__(self, ws_freq_name, Re_run, Re, Im_run, Im, phasequad):
        self.Re_run = Re_run

        self.ws_freq_name = ws_freq_name
        self.Re = Re
        if Im == "" and Im_run == "":
            self.Im = None
            self.Im_run = None
        else:
            self.Im = Im
            self.Im_run = Im_run
        self.phasequad = phasequad


FREQUENCY_EXTENSIONS ={"MOD":"mod", "RE":"Re", "IM":"Im", "MAXENT":"MaxEnt", "FFT":"FFT All" }


class FrequencyContext(object):

    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MonAnalysis
    """

    def __init__(self):
        self._maxEnt_freq = {}
        self._FFT_freq = {}
        self.plot_type = "None"

    @property
    def window_title(self):
        return "Frequency Domain Analysis"

    def add_maxEnt(self, run, ws_freq):
        self._maxEnt_freq[ws_freq] = MaxEnt(run, AnalysisDataService.retrieve(ws_freq))

    @property
    def maxEnt_freq(self):
        return list(self._maxEnt_freq.keys())

    def add_FFT(self, ws_freq_name, Re_run, Re, Im_run, Im, phasequad=False):
        self._FFT_freq[ws_freq_name] = FFT(ws_freq_name, Re_run, Re, Im_run, Im, phasequad=phasequad)

    @property
    def FFT_freq(self):
        return list(self._FFT_freq.keys())

    def _is_it_match(self, run, group, pair, fft_run, fft_group_or_pair):
        return int(run) == int(fft_run) and (fft_group_or_pair in group or fft_group_or_pair in pair)

    def get_frequency_workspace_names(self, run_list, group, pair, phasequad, frequency_type):
        # do MaxEnt first as it only has run number
        names = []
        for name, maxEnt in iteritems(self._maxEnt_freq):
            for runs in run_list:
                for run in runs:
                    if int(run) == int(maxEnt.run):
                        names.append(name)
        # do FFT
        for name, fft in iteritems(self._FFT_freq):
            for runs in run_list:
                for run in runs:
                    # check Re part
                    if self._is_it_match(run, group, pair, fft.Re_run, fft.Re) and name not in names:
                        names.append(name)
                    # check Im part
                    if fft.Im_run and self._is_it_match(run, group, pair, fft.Im_run, fft.Im) and name not in names:
                        names.append(name)
                    # do phaseQuad - will only have one run
                    if int(run) == int(fft.Re_run) and phasequad and fft.phasequad and name not in names:
                        names.append(name)
                    if fft.Im_run and int(run) == int(fft.Im_run) and phasequad and fft.phasequad and name not in names:
                        names.append(name)
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
