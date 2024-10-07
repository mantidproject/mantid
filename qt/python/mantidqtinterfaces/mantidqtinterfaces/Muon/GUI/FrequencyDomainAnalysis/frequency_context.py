# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from re import findall
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import convert_to_field, convert_to_freq
from mantid.simpleapi import RenameWorkspace

UNIT = "_unit_"
MHz = "MHz"
GAUSS = "Gauss"
FREQ = "Frequency"
FIELD = "Field"


class freq_and_field_ws(object):
    def __init__(self, ws_name, ws_unit):
        if ws_unit == MHz:
            self._freq = ws_name + UNIT + MHz
            self._field = convert_to_field(ws_name, ws_name + UNIT + GAUSS)
            RenameWorkspace(InputWorkspace=ws_name, OutputWorkspace=self._freq)
        elif ws_unit == GAUSS:
            self._field = ws_name + UNIT + GAUSS
            self._freq = convert_to_freq(ws_name, ws_name + UNIT + MHz)
            RenameWorkspace(InputWorkspace=ws_name, OutputWorkspace=self._field)

    def get_ws(self, x_label):
        if x_label == FIELD:
            return self._field
        # otherwise assume its freq
        return self._freq


class MaxEnt(object):
    def __init__(self, run, freq_field_ws):
        self.run = run
        self.ws = freq_field_ws


class FFT(object):
    # fft has two runs, one for Re and one for Im

    def __init__(self, freq_field_ws, Re_run, Re, Im_run, Im):
        self.Re_run = Re_run

        self.ws = freq_field_ws
        self.Re = Re
        if Im == "" and Im_run == "":
            self.Im = None
            self.Im_run = None
        else:
            self.Im = Im
            self.Im_run = Im_run


FREQUENCY_EXTENSIONS = {"MOD": "mod", "RE": "Re", "IM": "Im", "MAXENT": "MaxEnt", "FFT": "FFT All"}


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
        self.x_label = "Frequency"

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
        return [
            phase_table.workspace_name for phase_table in self._group_phase_tables[num_groups] if instrument in phase_table.workspace_name
        ]

    def add_maxEnt(self, run, maxent_ws):
        freq_field_ws = freq_and_field_ws(maxent_ws, GAUSS)
        self._maxEnt_freq[maxent_ws] = MaxEnt(run, freq_field_ws)

    @property
    def maxEnt_freq(self):
        return list(self._maxEnt_freq.keys())

    def add_FFT(self, ws_freq_name, Re_run, Re, Im_run, Im):
        freq_field_ws = freq_and_field_ws(ws_freq_name, MHz)
        self._FFT_freq[ws_freq_name] = FFT(freq_field_ws, Re_run, Re, Im_run, Im)

    @property
    def FFT_freq(self):
        return list(self._FFT_freq.keys())

    def unit(self):
        if self.x_label == FIELD:
            return GAUSS
        else:
            return MHz

    def get_ws_name(self, name):
        return name + UNIT + self.unit()

    def _is_it_match(self, run, group, pair, fft_run, fft_group_or_pair):
        return int(run) == int(fft_run) and (fft_group_or_pair in group or fft_group_or_pair in pair)

    def switch_units_in_name(self, name):
        if name is None:
            return None
        if MHz in name:
            return name.replace(MHz, GAUSS)
        elif GAUSS in name:
            return name.replace(GAUSS, MHz)
        else:
            return name

    def range(self):
        if self.x_label == FIELD:
            return [0.0, 1000.0]
        else:
            return [0.0, 30.0]

    def get_frequency_workspace_names(self, run_list, group, pair, frequency_type, x_label):
        # do MaxEnt first as it only has run number
        ws_list = {}
        for name, maxEnt in self._maxEnt_freq.items():
            for runs in run_list:
                for run in runs:
                    if int(run) == int(maxEnt.run):
                        ws_list[name] = maxEnt.ws
        # do FFT
        for name, fft in self._FFT_freq.items():
            for runs in run_list:
                for run in runs:
                    # check Re part
                    if self._is_it_match(run, group, pair, fft.Re_run, fft.Re) and name not in ws_list.keys():
                        ws_list[name] = fft.ws

                    # check Im part
                    if fft.Im_run and self._is_it_match(run, group, pair, fft.Im_run, fft.Im) and name not in ws_list.keys():
                        ws_list[name] = fft.ws
        if frequency_type == "All":
            return [freq_field.get_ws(x_label) for name, freq_field in ws_list.items()]
        elif frequency_type == FREQUENCY_EXTENSIONS["FFT"]:
            return [freq_field.get_ws(x_label) for name, freq_field in ws_list.items() if FREQUENCY_EXTENSIONS["MAXENT"] not in name]
        else:
            output = []
            count = 1
            """ if Re or Im then the count will be 2
            appears as part of the FFT name"""
            if len(frequency_type) == 2:
                count = 2
            for name, freq_field in ws_list.items():
                num = len(findall(frequency_type, name))
                if frequency_type in name and count == num:
                    output.append(freq_field.get_ws(x_label))
            return output
