# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import isis_powder.routines.common as common
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict
import numpy as np
from mantid.simpleapi import (
    NormaliseByCurrent,
    CropWorkspace,
    RebinToWorkspace,
    Minus,
    Divide,
    DiffractionFocussing,
    ConvertUnits,
    MergeRuns,
    GroupWorkspaces,
)
from mantid.api import AnalysisDataService


d_range_with_time = {
    "drange1": [11700, 51700],
    "drange2": [29400, 69400],
    "drange3": [47100, 87100],
    "drange4": [64800, 104800],
    "drange5": [82500, 122500],
    "drange6": [100200, 140200],
    "drange7": [117900, 157900],
    "drange8": [135500, 175500],
    "drange9": [153200, 193200],
    "drange10": [170900, 210900],
    "drange11": [188600, 228600],
    "drange12": [206300, 246300],
}

d_range_alice = {
    "drange1": [0.7, 2.5],
    "drange2": [2.1, 3.3],
    "drange3": [3.1, 4.3],
    "drange4": [4.1, 5.3],
    "drange5": [5.2, 6.2],
    "drange6": [6.2, 7.3],
    "drange7": [7.3, 8.3],
    "drange8": [8.3, 9.5],
    "drange9": [9.4, 10.6],
    "drange10": [10.4, 11.6],
    "drange11": [11.0, 12.5],
    "drange12": [12.0, 13.7],
}


class WORKSPACE_SUFFIX(object):
    CONTAINER_CORRECTED = "_container_corrected"
    MERGED = "_merged"
    FOCUSED = "_focused"
    GROUPED = "_grouped"


def get_osiris_d_range(run_ws):
    x_data = run_ws.dataX(0)
    x_range = [x_data[0], x_data[-1]]
    return list(d_range_with_time.keys())[list(d_range_with_time.values()).index(x_range)]


class DrangeData(object):
    def __init__(self, drange):
        self._drange = drange
        self._sample = []
        self._vanadium = ""
        self._empty = ""

    def get_drange(self):
        return self._drange

    def add_sample(self, ws_name):
        if ws_name not in self._sample:
            self._sample.append(ws_name)

    def get_samples(self):
        return self._sample

    def has_sample(self):
        return len(self._sample) >= 1

    def set_vanadium(self, ws_name):
        self._vanadium = ws_name
        return

    def set_empty(self, ws_name):
        self._empty = ws_name
        return

    def calibrate_vanadium(self, calibration_file=""):
        return self.calibrate_workspace(self._vanadium, calibration_file)

    def calibrate_workspace(self, ws, calibration_file=""):
        calibrate_ws = NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace="normalised", StoreInADS=False)
        if calibration_file:
            calibrate_ws = DiffractionFocussing(
                InputWorkspace=calibrate_ws, GroupingFileName=calibration_file, OutputWorkspace="focused", StoreInADS=False
            )

        calibrate_ws = ConvertUnits(
            InputWorkspace=calibrate_ws, Target="dSpacing", Emode="Indirect", AlignBins=True, OutputWorkspace="aligned", StoreInADS=False
        )

        calibrate_ws = CropWorkspace(
            InputWorkspace=calibrate_ws,
            XMin=d_range_alice[self._drange][0],
            XMax=d_range_alice[self._drange][1],
            OutputWorkspace="calibrated",
            StoreInADS=False,
        )
        return calibrate_ws

    def process_workspace(self, subtract_empty=False, vanadium_correct=False, focus_calibration_file=""):
        processed = []
        for sample in self._sample:
            outputname = sample + WORKSPACE_SUFFIX.FOCUSED
            if subtract_empty:
                sample = self.subtract_container_from_sample(sample)
            sample = self.calibrate_workspace(sample, focus_calibration_file)
            if vanadium_correct:
                van = self.calibrate_vanadium(focus_calibration_file)
                calib_van = RebinToWorkspace(WorkspaceToRebin=van, WorkspaceToMatch=sample, OutputWorkspace="van_rb", StoreInADS=False)
                sample = Divide(LHSWorkspace=sample, RHSWorkspace=calib_van, OutputWorkspace=outputname)
            AnalysisDataService.addOrReplace(outputname, sample)
            processed.append(sample)
        return processed

    def subtract_container_from_sample(self, sample):
        empty_rb = RebinToWorkspace(WorkspaceToRebin=self._empty, WorkspaceToMatch=sample, OutputWorkspace="empty_rb", StoreInADS=False)
        return Minus(
            LHSWorkspace=sample, RHSWorkspace=empty_rb, OutputWorkspace=sample + WORKSPACE_SUFFIX.CONTAINER_CORRECTED, StoreInADS=False
        )


def load_raw(run_number_string, drange_sets, group, inst, file_ext):
    """
    :param run_number_string: string of run numbers for the sample
    :param inst: The OSIRIS instrument object
    """
    input_ws_list = common.load_raw_files(run_number_string=run_number_string, instrument=inst, file_ext=file_ext)
    for ws in input_ws_list:
        drange = get_osiris_d_range(ws)
        if group == "sample":
            drange_sets[drange].add_sample(ws.name())
        elif group == "vanadium":
            drange_sets[drange].set_vanadium(ws.name())
        elif group == "empty":
            drange_sets[drange].set_empty(ws.name())
    _group_workspaces(input_ws_list, "OSIRIS" + run_number_string + WORKSPACE_SUFFIX.GROUPED)
    return input_ws_list


def run_diffraction_focussing(run_number, drange_sets, calfile, van_norm=False, subtract_empty=False):
    focused = []
    for drange in drange_sets:
        processed = drange_sets[drange].process_workspace(
            subtract_empty=subtract_empty, vanadium_correct=van_norm, focus_calibration_file=calfile
        )
        focused.extend(processed)
    _group_workspaces(focused, "OSIRIS" + run_number + WORKSPACE_SUFFIX.FOCUSED)
    return focused


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    empty_runs = _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_run_numbers")
    vanadium_runs = _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_run_numbers")

    grouping_file_name = inst_settings.grouping

    spline_identifier = []

    return create_run_details_object(
        run_number_string=run_number_string,
        inst_settings=inst_settings,
        is_vanadium_run=is_vanadium_run,
        splined_name_list=spline_identifier,
        grouping_file_name=grouping_file_name,
        empty_inst_run_number=empty_runs,
        vanadium_string=vanadium_runs,
    )


def get_van_runs_for_samples(run_number_string, inst_settings, drange_sets):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    van_numbers = ""
    for drange_set in drange_sets.values():
        if drange_set.has_sample():
            van_numbers += (
                _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_" + drange_set.get_drange()) + ","
            )
    return van_numbers[:-1]


def get_empty_runs_for_samples(run_number_string, inst_settings, drange_sets):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    empty_numbers = ""
    for drange_set in drange_sets.values():
        if drange_set.has_sample():
            empty_numbers += (
                _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_" + drange_set.get_drange()) + ","
            )
    return empty_numbers[:-1]


def get_vanadium_runs(inst_settings):
    all_run_numbers = get_cal_mapping_dict(inst_settings.run_number, inst_settings.cal_mapping_path)
    return [_assign_drange_vanadium("drange" + str(i + 1), all_run_numbers) for i in range(12)]


def get_empty_runs(inst_settings):
    all_run_numbers = get_cal_mapping_dict(inst_settings.run_number, inst_settings.cal_mapping_path)
    return [_assign_drange_empty("drange" + str(i + 1), all_run_numbers) for i in range(12)]


def _correct_drange_overlap(merged_ws, drange_sets):
    # Create scalar data to cope with where merge has combined overlapping data.
    data_x = (merged_ws.dataX(0)[1:] + merged_ws.dataX(0)[:-1]) / 2.0
    data_y = np.zeros(data_x.size)
    for drange in drange_sets:
        if drange_sets[drange].has_sample():
            for i in range(data_x.size):
                if d_range_alice[drange][0] <= data_x[i] <= d_range_alice[drange][1]:
                    data_y[i] += len(drange_sets[drange].get_samples())

    # apply scalar data to result workspace
    for i in range(0, merged_ws.getNumberHistograms()):
        merged_ws.setY(i, merged_ws.dataY(i) / data_y)
        merged_ws.setE(i, merged_ws.dataE(i) / data_y)
    return merged_ws


def _merge_dspacing_runs(run_number, drange_sets, ws_group):
    merged = MergeRuns(InputWorkspaces=ws_group, OutputWorkspace="OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED)
    return _correct_drange_overlap(merged, drange_sets)


def _group_workspaces(ws_list, output):
    GroupWorkspaces(InputWorkspaces=ws_list, outputWorkspace=output)


def _assign_drange_vanadium(drange, mapping):
    return "OSIRIS" + _get_run_numbers_for_key(current_mode_run_numbers=mapping, key="vanadium_" + drange)


def _assign_drange_empty(range, mapping):
    return "OSIRIS" + _get_run_numbers_for_key(current_mode_run_numbers=mapping, key="empty_" + range)


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key, append_to_error_message=err_message)
