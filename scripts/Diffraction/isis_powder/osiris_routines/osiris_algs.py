# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import isis_powder.routines.common as common
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict
import numpy as np
from mantid.kernel import logger
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
    ApplyDiffCal,
    ReplaceSpecialValues,
    ConjoinSpectra,
    CreateWorkspace,
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

    def get_samples_string(self):
        run_number_string = ""

        for s in self._sample:
            run_number_string += str(s) + ","

        return run_number_string[:-1]

    def has_sample(self):
        return len(self._sample) >= 1

    def set_vanadium(self, ws_name):
        self._vanadium = ws_name
        return

    def set_empty(self, ws_name):
        self._empty = ws_name
        return

    def calibrate_and_focus_vanadium(self, calibration_file=""):
        return self.calibrate_and_focus_workspace(self._vanadium, calibration_file)

    def calibrate_and_focus_workspace(self, ws, calibration_file=""):
        calibrate_ws = NormaliseByCurrent(InputWorkspace=ws, OutputWorkspace="__normalised")
        ApplyDiffCal(InstrumentWorkspace=calibrate_ws, CalibrationFile=calibration_file)
        calibrate_ws = ConvertUnits(
            InputWorkspace=calibrate_ws, Target="dSpacing", Emode="Elastic", AlignBins=False, OutputWorkspace="aligned", StoreInADS=False
        )
        if calibration_file:
            calibrate_ws = DiffractionFocussing(
                InputWorkspace=calibrate_ws, GroupingFileName=calibration_file, OutputWorkspace="focused", StoreInADS=False
            )
        calibrate_ws = CropWorkspace(
            InputWorkspace=calibrate_ws,
            XMin=d_range_alice[self._drange][0],
            XMax=d_range_alice[self._drange][1],
            OutputWorkspace="calibrated",
            StoreInADS=False,
        )
        return calibrate_ws

    def process_workspaces(self, subtract_empty=False, vanadium_correct=False, focus_calibration_file=""):
        if len(self._sample) == 0:
            return []

        processed = []
        sample_workspace = rebin_and_sum(self._sample)

        run_numbers = "OSIRIS" + ",".join([str(ws)[6:-4] for ws in self._sample])
        outputname = run_numbers + WORKSPACE_SUFFIX.FOCUSED
        AnalysisDataService.addOrReplace(outputname, sample_workspace)

        if subtract_empty:
            sample_workspace = self.subtract_container_from_sample(outputname)
        sample_workspace = self.calibrate_and_focus_workspace(sample_workspace, focus_calibration_file)
        if vanadium_correct:
            van = self.calibrate_and_focus_vanadium(focus_calibration_file)
            calib_van = RebinToWorkspace(
                WorkspaceToRebin=van, WorkspaceToMatch=sample_workspace, OutputWorkspace="van_rb", StoreInADS=False
            )
            sample_workspace = Divide(LHSWorkspace=sample_workspace, RHSWorkspace=calib_van, OutputWorkspace=outputname)
            sample_workspace = ReplaceSpecialValues(
                InputWorkspace=sample_workspace,
                NaNValue=0.0,
                InfinityValue=0.0,
                StoreInADS=False,
                EnableLogging=False,
            )

        AnalysisDataService.addOrReplace(outputname, sample_workspace)
        processed.append(sample_workspace)

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
            drange_sets[drange].add_sample(ws)
        elif group == "vanadium":
            drange_sets[drange].set_vanadium(ws)
        elif group == "empty":
            drange_sets[drange].set_empty(ws)
    _group_workspaces(input_ws_list, "OSIRIS" + run_number_string + WORKSPACE_SUFFIX.GROUPED)
    return input_ws_list


def rebin_and_sum(workspaces):
    """
    Rebins the specified list to the workspace with the smallest
    x-range in the list then sums all the workspaces together.

    :param workspaces: The list of workspaces to rebin to the smallest.
    :return:           The summed and rebinned workspace
    """
    if len(workspaces) == 1:
        return workspaces[0]

    smallest_idx, smallest_ws = min(enumerate(workspaces), key=lambda x: x[1].blocksize())

    rebinned_workspaces = []
    for idx, workspace in enumerate(workspaces):
        # Check whether this is the workspace with the smallest x-range.
        # No reason to rebin workspace to match itself.
        # NOTE: In the future this may append workspace.clone() - this will
        # occur in the circumstance that the input files do not want to be
        # removed from the ADS.
        if idx == smallest_idx:
            rebinned_workspaces.append(workspace)
        else:
            rebinned_workspaces.append(
                RebinToWorkspace(
                    WorkspaceToRebin=workspace,
                    WorkspaceToMatch=smallest_ws,
                    OutputWorkspace="rebinned",
                    StoreInADS=False,
                    EnableLogging=False,
                )
            )

    # sum will add together the proton charge in the sample logs
    # so when the NormalizeByCurrent is called we get an average of the workspaces
    return sum(rebinned_workspaces)


def run_diffraction_focussing(run_number, drange_sets, calfile, van_norm=False, subtract_empty=False):
    focused = []
    for drange in drange_sets:
        processed = drange_sets[drange].process_workspaces(
            subtract_empty=subtract_empty, vanadium_correct=van_norm, focus_calibration_file=calfile
        )
        focused.extend(processed)
    if len(focused) > 1:
        _group_workspaces(focused, "OSIRIS" + run_number + WORKSPACE_SUFFIX.FOCUSED)
    return focused


def create_drange_sets(run_number_string, inst, file_ext):
    run_number_list = common.generate_run_numbers(run_number_string=run_number_string)

    drange_sets = {}
    for run_number in run_number_list:
        ws_name = inst._generate_input_file_name(run_number=run_number, file_ext=file_ext if file_ext else "")
        ws = common.load_file(ws_name)
        drange = get_osiris_d_range(ws)

        if drange not in drange_sets:
            drange_sets[drange] = DrangeData(drange)

        drange_sets[drange].add_sample(run_number)

    return drange_sets


def get_run_details(run_number_string, inst_settings, is_vanadium_run, drange=None):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)

    if not drange:
        empty_can_runs = (
            _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_run_numbers")
            if inst_settings.subtract_empty_can
            else None
        )
        vanadium_runs = _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_run_numbers")
    else:
        empty_can_runs = get_empty_run_for_drange(all_run_numbers, drange) if inst_settings.subtract_empty_can else None
        vanadium_runs = get_van_run_for_drange(all_run_numbers, drange)

    grouping_file_name = inst_settings.grouping
    inst_settings.sample_empty = empty_can_runs

    return create_run_details_object(
        run_number_string=run_number_string,
        inst_settings=inst_settings,
        is_vanadium_run=is_vanadium_run,
        empty_inst_run_number=None,
        grouping_file_name=grouping_file_name,
        vanadium_string=vanadium_runs,
    )


def get_van_runs_for_samples(run_number_string, inst_settings, drange_sets):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    van_numbers = ""
    for drange_set in drange_sets.values():
        if drange_set.has_sample():
            van_numbers += get_van_run_for_drange(all_run_numbers, drange_set.get_drange()) + ","
    return van_numbers[:-1]


def get_van_run_for_drange(all_run_numbers, drange):
    return _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_" + drange)


def get_empty_runs_for_samples(run_number_string, inst_settings, drange_sets):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    empty_numbers = ""
    for drange_set in drange_sets.values():
        if drange_set.has_sample():
            empty_numbers += get_empty_run_for_drange(all_run_numbers, drange_set.get_drange()) + ","
    return empty_numbers[:-1]


def get_empty_run_for_drange(all_run_numbers, drange):
    return _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_" + drange)


def get_vanadium_runs(inst_settings):
    all_run_numbers = get_cal_mapping_dict(inst_settings.run_number, inst_settings.cal_mapping_path)
    return [_assign_drange_vanadium("drange" + str(i + 1), all_run_numbers) for i in range(12)]


def get_empty_runs(inst_settings):
    all_run_numbers = get_cal_mapping_dict(inst_settings.run_number, inst_settings.cal_mapping_path)
    return [_assign_drange_empty("drange" + str(i + 1), all_run_numbers) for i in range(12)]


def _correct_drange_overlap(merged_ws, drange_sets):
    for i in range(0, merged_ws.getNumberHistograms()):
        # Create scalar data to cope with where merge has combined overlapping data.
        data_x = (merged_ws.dataX(i)[1:] + merged_ws.dataX(i)[:-1]) / 2.0
        data_y = np.zeros(data_x.size)
        for drange in drange_sets:
            if drange_sets[drange].has_sample():
                for j in range(data_x.size):
                    if d_range_alice[drange][0] <= data_x[j] <= d_range_alice[drange][1]:
                        data_y[j] += 1

        for z in range(data_y.size):
            if data_y[z] == 0:
                data_y[z] = 1

        # apply scalar data to result workspace
        merged_ws.setY(i, merged_ws.dataY(i) / data_y)
        merged_ws.setE(i, merged_ws.dataE(i) / data_y)

    return merged_ws


def merge_dspacing_runs(focussed_runs, drange_sets, run_number, split=False):
    if len(focussed_runs) == 1:
        if split:
            grouped_spectra = GroupWorkspaces(
                InputWorkspaces=focussed_runs, OutputWorkspace="OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED
            )
        else:
            grouped_spectra = GroupWorkspaces(
                InputWorkspaces=focussed_runs[0], OutputWorkspace="OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED
            )
        return [_correct_drange_overlap(grouped_spectra, drange_sets)]

    extracted_spectra = focussed_runs
    if split:
        extracted_spectra = [common.extract_ws_spectra(ws) for ws in focussed_runs]

    have_same_spectra_count = len({len(spectra) for spectra in extracted_spectra}) == 1
    if not have_same_spectra_count:
        logger.warning("Cannot merge focussed workspaces with different number of spectra")
        return [ws for ws in focussed_runs]

    output_name = "OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED

    # Group workspaces located at the same index
    matched_spectra = [list(spectra) for spectra in zip(*extracted_spectra)]
    # Merge workspaces located at the same index
    merged_spectra = [MergeRuns(InputWorkspaces=spectra, OutputWorkspace=f"merged_{idx}") for idx, spectra in enumerate(matched_spectra)]

    max_x_size = max([spectra.dataX(0).size for spectra in merged_spectra])
    max_y_size = max([spectra.dataY(0).size for spectra in merged_spectra])
    max_e_size = max([spectra.dataE(0).size for spectra in merged_spectra])

    for i in range(len(merged_spectra)):
        if (
            merged_spectra[i].dataX(0).size != max_x_size
            or merged_spectra[i].dataY(0).size != max_y_size
            or merged_spectra[i].dataE(0).size != max_e_size
        ):
            dataX = merged_spectra[i].dataX(0)
            dataY = merged_spectra[i].dataY(0)
            dataE = merged_spectra[i].dataE(0)

            dataX = np.append(dataX, [dataX[-1]] * (max_x_size - dataX.size))
            dataY = np.append(dataY, [0] * (max_y_size - dataY.size))
            dataE = np.append(dataE, [0] * (max_e_size - dataE.size))

            merged_spectra[i] = CreateWorkspace(
                NSpec=1,
                DataX=dataX,
                DataY=dataY,
                DataE=dataE,
                UnitX=merged_spectra[i].getAxis(0).getUnit().unitID(),
                Distribution=merged_spectra[i].isDistribution(),
                ParentWorkspace=merged_spectra[i].name(),
                OutputWorkspace=merged_spectra[i].name(),
            )

    input_workspaces_str = ",".join([ws.name() for ws in merged_spectra])
    ConjoinSpectra(InputWorkspaces=input_workspaces_str, OutputWorkspace=output_name)

    if split:
        common.remove_intermediate_workspace([ws for ws_group in matched_spectra for ws in ws_group])
    common.remove_intermediate_workspace([ws for ws in merged_spectra])

    joined_spectra = AnalysisDataService[output_name]
    return [_correct_drange_overlap(joined_spectra, drange_sets)]


def _group_workspaces(ws_list, output):
    GroupWorkspaces(InputWorkspaces=ws_list, outputWorkspace=output)


def _assign_drange_vanadium(drange, mapping):
    return "OSIRIS" + _get_run_numbers_for_key(current_mode_run_numbers=mapping, key="vanadium_" + drange)


def _assign_drange_empty(range, mapping):
    return "OSIRIS" + _get_run_numbers_for_key(current_mode_run_numbers=mapping, key="empty_" + range)


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key, append_to_error_message=err_message)
