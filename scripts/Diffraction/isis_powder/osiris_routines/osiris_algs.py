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
    RebinToWorkspace,
    MergeRuns,
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


def create_drange_sets(run_number_string, inst, file_ext):
    run_number_list = common.generate_run_numbers(run_number_string=run_number_string)

    drange_sets = {}
    for run_number in run_number_list:
        ws = common.load_raw_files(run_number_string=run_number, instrument=inst, file_ext=file_ext)[0]
        drange = get_osiris_d_range(ws)

        if drange not in drange_sets:
            drange_sets[drange] = DrangeData(drange)

        drange_sets[drange].add_sample(run_number)

    return drange_sets


def get_run_details(run_number_string, inst_settings, is_vanadium_run, drange=None):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)

    if not drange:
        empty_can_runs = (
            _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_can_run_numbers")
            if inst_settings.subtract_empty_can
            else None
        )
        vanadium_runs = _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_run_numbers")
    else:
        empty_can_runs = (
            get_empty_can_run_for_drange(all_run_numbers, drange)
            if (inst_settings.subtract_empty_can or inst_settings.empty_can_subtraction_method == "PaalmanPings")
            else None
        )
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


def get_van_run_for_drange(all_run_numbers, drange):
    return _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_" + drange)


def get_empty_can_run_for_drange(all_run_numbers, drange):
    return _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_" + drange)


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


def merge_dspacing_runs(focussed_runs, drange_sets, run_number):
    if len(focussed_runs) == 1:
        input_workspaces_str = ",".join([ws.name() for ws in focussed_runs[0]])
        ConjoinSpectra(InputWorkspaces=input_workspaces_str, OutputWorkspace="OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED)

        joined_spectra = AnalysisDataService["OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED]
        return [_correct_drange_overlap(joined_spectra, drange_sets)]

    have_same_spectra_count = len({len(spectra) for spectra in focussed_runs}) == 1
    if not have_same_spectra_count:
        logger.warning("Cannot merge focussed workspaces with different number of spectra")
        return [ws for ws in focussed_runs]

    output_name = "OSIRIS" + run_number + WORKSPACE_SUFFIX.MERGED

    # Group workspaces located at the same index
    matched_spectra = [list(spectra) for spectra in zip(*focussed_runs)]
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

    common.remove_intermediate_workspace([ws for ws in merged_spectra])

    joined_spectra = AnalysisDataService[output_name]
    return [_correct_drange_overlap(joined_spectra, drange_sets)]


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key, append_to_error_message=err_message)
