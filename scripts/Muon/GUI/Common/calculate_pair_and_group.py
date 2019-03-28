# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import Muon.GUI.Common.utilities.algorithm_utils as algorithm_utils


def calculate_group_data(context, group_name, run, rebin):
    processed_data = _run_pre_processing(context, run, rebin)

    params = _get_MuonGroupingCounts_parameters(context, group_name, run)
    params["InputWorkspace"] = processed_data
    group_data = algorithm_utils.run_MuonGroupingCounts(params)

    return group_data


def calculate_pair_data(context, pair_name, run, rebin):
    processed_data = _run_pre_processing(context, run, rebin)

    params = _get_MuonPairingAsymmetry_parameters(context, pair_name, run)
    params["InputWorkspace"] = processed_data
    pair_data = algorithm_utils.run_MuonPairingAsymmetry(params)

    return pair_data


def estimate_group_asymmetry_data(context, group_name, run, rebin):
    processed_data = _run_pre_processing(context, run, rebin)

    params = _get_MuonGroupingAsymmetry_parameters(context, group_name, run)
    params["InputWorkspace"] = processed_data
    group_asymmetry = algorithm_utils.run_MuonGroupingAsymmetry(params)

    return group_asymmetry


def _run_pre_processing(context, run, rebin):
    params = _get_pre_processing_params(context, run, rebin)
    params["InputWorkspace"] = context.data_context.loaded_workspace_as_group(run)
    processed_data = algorithm_utils.run_MuonPreProcess(params)
    return processed_data


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
            time_offset = context.data_context.get_loaded_data_for_run(run)["TimeZero"] - context.gui_context['TimeZero']
        pre_process_params["TimeOffset"] = time_offset
    except KeyError:
        pass

    if rebin:
        _setup_rebin_options(context, pre_process_params, run)

    try:
        if context.gui_context['DeadTimeSource'] == 'FromFile':
            dead_time_table = context.data_context.get_loaded_data_for_run(run)["DataDeadTimeTable"]
        elif context.gui_context['DeadTimeSource'] == 'FromADS':
            dead_time_table = context.gui_context['DeadTimeTable']
        else:
            dead_time_table = None

        if dead_time_table is not None:
            pre_process_params["DeadTimeTable"] = dead_time_table
    except KeyError:
        pass

    return pre_process_params


def _setup_rebin_options(context, pre_process_params, run):
    try:
        if context.gui_context['RebinType'] == 'Variable' and context.gui_context["RebinVariable"]:
            pre_process_params["RebinArgs"] = context.gui_context["RebinVariable"]
    except KeyError:
        pass

    try:
        if context.gui_context['RebinType'] == 'Fixed' and context.gui_context["RebinFixed"]:
            x_data = context.data_context._loaded_data.get_data(run=run, instrument=context.dainstrument
                                                                )['workspace']['OutputWorkspace'][0].workspace.dataX(0)
            original_step = x_data[1] - x_data[0]
            pre_process_params["RebinArgs"] = float(context.gui_context["RebinFixed"]) * original_step
    except KeyError:
        pass


def _get_MuonGroupingCounts_parameters(context, group_name, run):
    params = {}
    if context.data_context.is_multi_period() and 'SummedPeriods' in context.gui_context:
        summed_periods = context.gui_context["SummedPeriods"]
        params["SummedPeriods"] = summed_periods
    else:
        params["SummedPeriods"] = "1"

    if context.data_context.is_multi_period() and 'SubtractedPeriods' in context.gui_context:
        subtracted_periods = context.gui_context["SubtractedPeriods"]
        params["SubtractedPeriods"] = subtracted_periods
    else:
        params["SubtractedPeriods"] = ""

    group = context.group_pair_context[group_name]
    if group:
        params["GroupName"] = group_name
        params["Grouping"] = ",".join([str(i) for i in group.detectors])

    return params


def _get_MuonGroupingAsymmetry_parameters(context, group_name, run):
    params = {}

    if 'GroupRangeMin' in context.gui_context:
        params['AsymmetryTimeMin'] = context.gui_context['GroupRangeMin']
    else:
        params['AsymmetryTimeMin'] = context.data_context.get_loaded_data_for_run(run)["FirstGoodData"]

    if 'GroupRangeMax' in context.gui_context:
        params['AsymmetryTimeMax'] = context.gui_context['GroupRangeMax']
    else:
        params['AsymmetryTimeMax'] = max(context.data_context.get_loaded_data_for_run(run)['OutputWorkspace'][0].workspace.dataX(0))

    if context.data_context.is_multi_period() and 'SummedPeriods' in context.gui_context:
        summed_periods = context.gui_context["SummedPeriods"]
        params["SummedPeriods"] = summed_periods
    else:
        params["SummedPeriods"] = "1"

    if context.data_context.is_multi_period() and 'SubtractedPeriods' in context.gui_context:
        subtracted_periods = context.gui_context["SubtractedPeriods"]
        params["SubtractedPeriods"] = subtracted_periods
    else:
        params["SubtractedPeriods"] = ""

    group = context.group_pair_context[group_name]
    if group:
        params["GroupName"] = group_name
        params["Grouping"] = ",".join([str(i) for i in group.detectors])

    return params


def _get_MuonPairingAsymmetry_parameters(context, pair_name, run):
    params = {}
    if context.data_context.is_multi_period() and 'SummedPeriods' in context.gui_context:
        summed_periods = context.gui_context["SummedPeriods"]
        params["SummedPeriods"] = summed_periods
    else:
        params["SummedPeriods"] = "1"

    if context.data_context.is_multi_period() and 'SubtractedPeriods' in context.gui_context:
        subtracted_periods = context.gui_context["SubtractedPeriods"]
        params["SubtractedPeriods"] = subtracted_periods
    else:
        params["SubtractedPeriods"] = ""

    pair = context.group_pair_context[pair_name]

    if pair:
        params["SpecifyGroupsManually"] = True
        params["PairName"] = str(pair_name)
        detectors1 = ",".join([str(i) for i in context.group_pair_context[pair.forward_group].detectors])
        detectors2 = ",".join([str(i) for i in context.group_pair_context[pair.backward_group].detectors])
        params["Group1"] = detectors1
        params["Group2"] = detectors2
        params["Alpha"] = str(pair.alpha)

    return params
