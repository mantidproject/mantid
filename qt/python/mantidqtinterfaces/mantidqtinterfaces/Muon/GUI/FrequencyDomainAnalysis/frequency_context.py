# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from re import findall
from mantid import AnalysisDataService


class MaxEnt(object):

    def __init__(self, run, ws_freq):
        self.run = run
        self.ws_freq = ws_freq


class FFT(object):
    # fft has two runs, one for Re and one for Im

    def __init__(self, ws_freq_name, Re_run, Re, Im_run, Im):
        self.Re_run = Re_run

        self.ws_freq_name = ws_freq_name
        self.Re = Re
        if Im == "" and Im_run == "":
            self.Im = None
            self.Im_run = None
        else:
            self.Im = Im
            self.Im_run = Im_run


FREQUENCY_EXTENSIONS ={"MOD":"mod", "RE":"Re", "IM":"Im", "MAXENT":"MaxEnt", "FFT":"FFT All" }


class FrequencyContext(object):

    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MuonAnalysis
    """

    def __init__(self):
        self._maxEnt_freq = {}
        self._FFT_freq = {}
        self.plot_type = "All"
        self._group_phase_tables = {}

    @property
    def window_title(self):
        return "Frequency Domain Analysis"

    def add_group_phase_table(self, table, num_groups):
        if num_groups not in self._group_phase_tables.keys():
            self._group_phase_tables[num_groups] = [table]
        elif table.workspace_name not in [table.workspace_name for table in self._group_phase_tables[num_groups]]:
            self._group_phase_tables[num_groups] += [table]

    def get_group_phase_tables(self, num_groups, instrument):
        if num_groups not in self._group_phase_tables.keys():
            return []
        return [phase_table.workspace_name for phase_table in self._group_phase_tables[num_groups]
                if instrument in phase_table.workspace_name]

    def add_maxEnt(self, run, ws_freq):
        self._maxEnt_freq[ws_freq] = MaxEnt(run, AnalysisDataService.retrieve(ws_freq))

    @property
    def maxEnt_freq(self):
        return list(self._maxEnt_freq.keys())

    def add_FFT(self, ws_freq_name, Re_run, Re, Im_run, Im):
        self._FFT_freq[ws_freq_name] = FFT(ws_freq_name, Re_run, Re, Im_run, Im)

    @property
    def FFT_freq(self):
        return list(self._FFT_freq.keys())

    def _is_it_match(self, run, group, pair, fft_run, fft_group_or_pair):
        return int(run) == int(fft_run) and (fft_group_or_pair in group or fft_group_or_pair in pair)

    def get_frequency_workspace_names(self, run_list, group, pair, frequency_type):
        # do MaxEnt first as it only has run number
        names = []
        for name, maxEnt in self._maxEnt_freq.items():
            for runs in run_list:
                for run in runs:
                    if int(run) == int(maxEnt.run):
                        names.append(name)
        # do FFT
        for name, fft in self._FFT_freq.items():
            for runs in run_list:
                for run in runs:
                    # check Re part
                    if self._is_it_match(run, group, pair, fft.Re_run, fft.Re) and name not in names:
                        names.append(name)
                    # check Im part
                    if fft.Im_run and self._is_it_match(run, group, pair, fft.Im_run, fft.Im) and name not in names:
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
