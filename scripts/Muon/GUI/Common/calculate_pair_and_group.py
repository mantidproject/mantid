# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import Muon.GUI.Common.utilities.algorithm_utils as algorithm_utils


def calculate_group_data(context, group_name):
    processed_data = _run_pre_processing(context)

    params = _get_MuonGroupingCounts_parameters(context, group_name)
    params["InputWorkspace"] = processed_data
    group_data = algorithm_utils.run_MuonGroupingCounts(params)

    return group_data


def calculate_pair_data(context, pair_name):
    processed_data = _run_pre_processing(context)

    params = _get_MuonPairingAsymmetry_parameters(context, pair_name)
    params["InputWorkspace"] = processed_data
    pair_data = algorithm_utils.run_MuonPairingAsymmetry(params)

    return pair_data


def _run_pre_processing(context):
    params = _get_pre_processing_params(context)
    params["InputWorkspace"] = context.loaded_workspace
    processed_data = algorithm_utils.run_MuonPreProcess(params)
    return processed_data


def _get_pre_processing_params(context):
    pre_process_params = {}
    try:
        time_min = context.loaded_data["FirstGoodData"]
        pre_process_params["TimeMin"] = time_min
    except KeyError:
        pass

    try:
        rebin_args = context.loaded_data["Rebin"]
        pre_process_params["RebinArgs"] = rebin_args
    except KeyError:
        pass

    try:
        time_offset = context.loaded_data["TimeZero"]
        pre_process_params["TimeOffset"] = time_offset
    except KeyError:
        pass

    try:
        dead_time_table = context.dead_time_table
        if dead_time_table is not None:
            pre_process_params["DeadTimeTable"] = dead_time_table
    except KeyError:
        pass

    return pre_process_params


def _get_MuonGroupingCounts_parameters(context, group_name):
    params = {}
    try:
        summed_periods = context.loaded_data["SummedPeriods"]
        params["SummedPeriods"] = str(summed_periods)
    except KeyError:
        params["SummedPeriods"] = "1"

    try:
        subtracted_periods = context.loaded_data["SubtractedPeriods"]
        params["SubtractedPeriods"] = str(subtracted_periods)
    except KeyError:
        params["SubtractedPeriods"] = ""

    group = context._groups.get(group_name, None)
    if group:
        params["GroupName"] = group_name
        params["Grouping"] = ",".join([str(i) for i in group.detectors])

    return params


def _get_MuonPairingAsymmetry_parameters(context, pair_name):
    params = {}
    try:
        summed_periods = context.loaded_data["SummedPeriods"]
        params["SummedPeriods"] = str(summed_periods)
    except KeyError:
        params["SummedPeriods"] = "1"

    try:
        subtracted_periods = context.loaded_data["SubtractedPeriods"]
        params["SubtractedPeriods"] = str(subtracted_periods)
    except KeyError:
        params["SubtractedPeriods"] = ""

    pair = context._pairs.get(pair_name, None)

    if pair:
        params["SpecifyGroupsManually"] = True
        params["PairName"] = str(pair_name)
        detectors1 = ",".join([str(i) for i in context._groups[pair.forward_group].detectors])
        detectors2 = ",".join([str(i) for i in context._groups[pair.backward_group].detectors])
        params["Group1"] = detectors1
        params["Group2"] = detectors2
        params["Alpha"] = str(pair.alpha)

    return params
