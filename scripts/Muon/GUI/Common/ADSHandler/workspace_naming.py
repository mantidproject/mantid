# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import re
from mantid.api import AnalysisDataService, WorkspaceGroup


def get_raw_data_workspace_name(context, run, period='1'):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + "_raw_data" + "_period_" + period
    else:
        return context.data_context._base_run_name(run) + "_raw_data"


def get_group_data_workspace_name(context, group_name, run, rebin):
    if context.data_context.is_multi_period():
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + \
            "; Counts; Periods; " + context.gui_context.period_string(run) + ";"
    else:
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + "; Counts;"

    if rebin:
        name += ' Rebin;'

    name += ' #1'

    return name


def get_group_asymmetry_name(context, group_name, run, rebin):
    if context.data_context.is_multi_period():
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + \
            "; Asymmetry; Periods; " + context.gui_context.period_string(run) + ";"
    else:
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + "; Asymmetry;"

    if rebin:
        name += ' Rebin;'

    name += ' #1'

    return name


def get_pair_data_workspace_name(context, pair_name, run, rebin):
    if context.data_context.is_multi_period():
        name = context.data_context._base_run_name(run) + "; Pair Asym; " + pair_name + "; Periods; " \
            + context.gui_context.period_string(run) + ";"
    else:
        name = context.data_context._base_run_name(run) + "; Pair Asym; " + pair_name + ";"

    if rebin:
        name += ' Rebin;'

    name += ' #1'

    return name


def get_base_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context.base_directory + "/" + context.data_context._base_run_name(run) + "/"
    else:
        return context.data_context.base_directory + "/" + context.data_context._base_run_name(run) + "/"


def get_raw_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + "; Raw Data/"
    else:
        return context.data_context._base_run_name(run) + " Raw Data/"


def get_cached_data_directory(context, run):
    if context.is_multi_period():
        return context._base_run_name() + " Period " + context.period_string(run) + "; Cached/"
    else:
        return context._base_run_name() + " Cached/"


def get_group_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + " Period " + context.gui_context.period_string(
            run) + "; Groups/"
    else:
        return context.data_context._base_run_name(run) + " Groups/"


def get_pair_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + " Period " + context.gui_context.period_string(
            run) + "; Pairs/"
    else:
        return context.data_context._base_run_name(run) + " Pairs/"


# def get_phase_quad_workspace_name(context, run, period):
#     return get_raw_data_workspace_name(context, run, period=str(period)) + " (PhaseQuad)"


def calculate_base_name_and_group_for_phase_table(context, table_name):
    run = re.search('[0-9]+', table_name).group()
    base_name = table_name + '; ' + context.phase_context.options_dict['forward_group']
    base_name += ', ' + context.phase_context.options_dict['backward_group']

    group = context.data_context._base_run_name(run) + ' PhaseTables'

    if not AnalysisDataService.doesExist(group) or type(AnalysisDataService.retrieve(group)) is not WorkspaceGroup:
        new_group = WorkspaceGroup()
        AnalysisDataService.addOrReplace(group, new_group)
        AnalysisDataService.addToGroup(context.data_context._base_run_name(run), group)

    return base_name, group


def calculate_base_name_and_group_for_phasequad(context, input_workspace, phase_table):
    base_name = input_workspace.split('_')[0] + '; PhaseQuad; ' \
                + phase_table

    run = re.search('[0-9]+', input_workspace).group()
    group = context.data_context._base_run_name(run) + ' PhaseTables'

    if not AnalysisDataService.doesExist(group) or type(AnalysisDataService.retrieve(group)) is not WorkspaceGroup:
        new_group = WorkspaceGroup()
        AnalysisDataService.addOrReplace(group, new_group)
        AnalysisDataService.addToGroup(context.data_context._base_run_name(run), group)

    return base_name, group


def get_phase_table_workspace_name(raw_workspace, forward_group, backward_group):
    workspace_name = raw_workspace.replace('_raw_data', '; PhaseTable')
    workspace_name += '; ' + forward_group + ', ' + backward_group
    return workspace_name


def get_base_run_name(run, instrument):
    if isinstance(run, int):
        return str(instrument) + str(run)
    else:
        return str(instrument) + run


def get_phase_table_workspace_group_name(insertion_workspace_name, instrument):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group =  get_base_run_name(run, instrument) + ' PhaseTables/'

    return group


def get_fft_workspace_group_name(insertion_workspace_name, instrument):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + ' FFT/'

    return group


def get_phase_quad_workspace_name(input_workspace, phase_table):
    return input_workspace.replace('_raw_data', '; PhaseQuad') + ' ' + phase_table


def get_fitting_workspace_name(base_name):
    return base_name + '; fit_information'


def get_fft_workspace_name(input_workspace):
    return input_workspace + '; FFT'


def get_maxent_workspace_name(input_workspace):
    return input_workspace + '; MaxEnt'


def get_maxent_workspace_group_name(insertion_workspace_name, instrument):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group =  get_base_run_name(run, instrument) + ' Maxent/'

    return group
