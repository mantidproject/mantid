# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import Muon.GUI.Common.utilities.algorithm_utils as algorithm_utils
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.Common.muon_pair import MuonPair
from typing import Iterable


def calculate_group_data(context, group, run, rebin, workspace_name, periods):
    processed_data = get_pre_process_workspace_name(run, context.data_context.instrument)

    params = _get_MuonGroupingCounts_parameters(group, periods)
    params["InputWorkspace"] = processed_data

    group_data = algorithm_utils.run_MuonGroupingCounts(params, workspace_name)

    return group_data


def calculate_pair_data(pair: MuonPair, forward_group: str, backward_group: str, output_workspace_name: str):
    params = _get_MuonPairingAsymmetry_parameters(pair, forward_group, backward_group)
    pair_data = algorithm_utils.run_MuonPairingAsymmetry(params, output_workspace_name)

    return pair_data


def estimate_group_asymmetry_data(context, group, run, rebin, workspace_name, unormalised_workspace_name, periods):
    processed_data = get_pre_process_workspace_name(run, context.data_context.instrument)

    params = _get_MuonGroupingAsymmetry_parameters(context, group, run, periods)
    params["InputWorkspace"] = processed_data
    group_asymmetry, group_asymmetry_unnorm = algorithm_utils.run_MuonGroupingAsymmetry(params, workspace_name,
                                                                                        unormalised_workspace_name)

    return group_asymmetry, group_asymmetry_unnorm


def run_pre_processing(context, run, rebin):
    params = _get_pre_processing_params(context, run, rebin)
    params["InputWorkspace"] = context.data_context.loaded_workspace_as_group(run)
    processed_data = algorithm_utils.run_MuonPreProcess(params)
    return processed_data


def get_pre_process_workspace_name(run: Iterable[int], instrument: str) -> str:
    workspace_name = "".join(["__", instrument, run_list_to_string(run), "_pre_processed_data"])
    return workspace_name


def _get_pre_processing_params(context, run, rebin):
    pre_process_params = {}

    try:
        if context.gui_context['FirstGoodDataFromFile']:
            time_min = context.data_context.get_loaded_data_for_run(run)["FirstGoodData"]
        else:
            time_min = context.gui_context['FirstGoodData']
        pre_process_params["TimeMin"] = time_min
    except KeyError:
        pass

    try:
        if context.gui_context['TimeZeroFromFile']:
            time_offset = 0.0
        else:
            time_offset = context.data_context.get_loaded_data_for_run(run)["TimeZero"] - context.gui_context[
                'TimeZero']
        pre_process_params["TimeOffset"] = time_offset
    except KeyError:
        pass

    if rebin:
        _setup_rebin_options(context, pre_process_params, run)

    try:
        dead_time_source = context.corrections_context.dead_time_source
        if dead_time_source == "FromFile":
            dead_time_table = context.data_context.get_loaded_data_for_run(run)["DataDeadTimeTable"]
        elif dead_time_source == "FromADS":
            dead_time_table = context.corrections_context.dead_time_table_name
        else:
            dead_time_table = None

        if dead_time_table is not None:
            pre_process_params["DeadTimeTable"] = dead_time_table
    except KeyError:
        pass

    pre_process_params["OutputWorkspace"] = get_pre_process_workspace_name(run, context.data_context.instrument)

    return pre_process_params


def _setup_rebin_options(context, pre_process_params, run):
    try:
        if context.gui_context['RebinType'] == 'Variable' and context.gui_context["RebinVariable"]:
            pre_process_params["RebinArgs"] = context.gui_context["RebinVariable"]
    except KeyError:
        pass

    try:
        if context.gui_context['RebinType'] == 'Fixed' and context.gui_context["RebinFixed"]:
            x_data = context.data_context._loaded_data.get_data(run=run, instrument=context.data_context.instrument
                                                                )['workspace']['OutputWorkspace'][0].workspace.dataX(0)
            original_step = x_data[1] - x_data[0]
            pre_process_params["RebinArgs"] = float(context.gui_context["RebinFixed"]) * original_step
    except KeyError:
        pass


def _get_MuonGroupingCounts_parameters(group, periods):
    params = {}
    params["SummedPeriods"] = periods

    if group:
        params["GroupName"] = group.name
        params["Grouping"] = ",".join([str(i) for i in group.detectors])

    return params


def _get_MuonGroupingAsymmetry_parameters(context, group, run, periods):
    params = {}

    if 'GroupRangeMin' in context.gui_context:
        params['AsymmetryTimeMin'] = context.gui_context['GroupRangeMin']
    else:
        params['AsymmetryTimeMin'] = context.data_context.get_loaded_data_for_run(run)["FirstGoodData"]

    if 'GroupRangeMax' in context.gui_context:
        params['AsymmetryTimeMax'] = context.gui_context['GroupRangeMax']
    else:
        params['AsymmetryTimeMax'] = max(
            context.data_context.get_loaded_data_for_run(run)['OutputWorkspace'][0].workspace.dataX(0))

    params["SummedPeriods"] = periods

    if group:
        params["GroupName"] = group.name
        params["Grouping"] = ",".join([str(i) for i in group.detectors])

    return params


def _get_MuonPairingAsymmetry_parameters(pair: MuonPair, forward_group: str, backward_group: str):
    params = {}

    if pair:
        params["SpecifyGroupsManually"] = False
        params["PairName"] = pair.name
        params["InputWorkspace1"] = forward_group
        params["InputWorkspace2"] = backward_group
        params["Alpha"] = str(pair.alpha)

    return params
