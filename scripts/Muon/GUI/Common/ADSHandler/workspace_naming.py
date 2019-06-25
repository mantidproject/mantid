# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import re


def get_raw_data_workspace_name(context, run, period='1'):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + "_raw_data" + "_period_" + period + context.workspace_suffix
    else:
        return context.data_context._base_run_name(run) + "_raw_data"  + context.workspace_suffix


def get_group_data_workspace_name(context, group_name, run, rebin):
    if context.data_context.is_multi_period():
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + \
            "; Counts; Periods; " + context.gui_context.period_string(run) + ";"
    else:
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + "; Counts;"

    if rebin:
        name += ' Rebin;'

    name += context.workspace_suffix

    return name


def get_group_asymmetry_name(context, group_name, run, rebin):
    if context.data_context.is_multi_period():
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + \
            "; Asymmetry; Periods; " + context.gui_context.period_string(run) + ";"
    else:
        name = context.data_context._base_run_name(run) + "; Group; " + group_name + "; Asymmetry;"

    if rebin:
        name += ' Rebin;'

    name += context.workspace_suffix

    return name


def get_pair_data_workspace_name(context, pair_name, run, rebin):
    if context.data_context.is_multi_period():
        name = context.data_context._base_run_name(run) + "; Pair Asym; " + pair_name + "; Periods; " \
            + context.gui_context.period_string(run) + ";"
    else:
        name = context.data_context._base_run_name(run) + "; Pair Asym; " + pair_name + ";"

    if rebin:
        name += ' Rebin;'

    name += context.workspace_suffix

    return name


def get_base_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context.base_directory + "/" + context.data_context._base_run_name(run) + context.workspace_suffix + "/"
    else:
        return context.data_context.base_directory + "/" + context.data_context._base_run_name(run) + context.workspace_suffix + "/"


def get_raw_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + "; Raw Data" + context.workspace_suffix + "/"
    else:
        return context.data_context._base_run_name(run) + " Raw Data" + context.workspace_suffix + "/"


def get_group_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + " Period " + context.gui_context.period_string(
            run) + "; Groups" + context.workspace_suffix + "/"
    else:
        return context.data_context._base_run_name(run) + " Groups" + context.workspace_suffix + "/"


def get_pair_data_directory(context, run):
    if context.data_context.is_multi_period():
        return context.data_context._base_run_name(run) + " Period " + context.gui_context.period_string(
            run) + "; Pairs" + context.workspace_suffix + "/"
    else:
        return context.data_context._base_run_name(run) + " Pairs" + context.workspace_suffix + "/"


def get_phase_table_workspace_name(raw_workspace, forward_group, backward_group):
    workspace_name = raw_workspace.replace('_raw_data', '; PhaseTable')
    workspace_name += '; ' + forward_group + ', ' + backward_group
    return workspace_name


def get_base_run_name(run, instrument):
    if isinstance(run, int):
        return str(instrument) + str(run)
    else:
        return str(instrument) + run


def get_phase_table_workspace_group_name(insertion_workspace_name, instrument, workspace_suffix):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + ' Phase Tab' + workspace_suffix + '/'

    return group


def get_fft_workspace_group_name(insertion_workspace_name, instrument, workspace_suffix):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + ' FFT' + workspace_suffix + '/'

    return group


def get_phase_quad_workspace_name(input_workspace, phase_table):
    return input_workspace.replace('_raw_data', '; PhaseQuad') + ' ' + phase_table


def get_fitting_workspace_name(base_name):
    return base_name + '; fit_information'


def get_fft_workspace_name(input_workspace, imaginary_input_workspace):
    if imaginary_input_workspace:
        return 'FFT; Re ' + input_workspace + '; Im ' + imaginary_input_workspace
    else:
        return 'FFT; Re ' + input_workspace


def get_maxent_workspace_name(input_workspace):
    return input_workspace + '; MaxEnt'


def get_maxent_workspace_group_name(insertion_workspace_name, instrument, workspace_suffix):
    run = re.search('[0-9]+', insertion_workspace_name).group()
    group = get_base_run_name(run, instrument) + ' Maxent' + workspace_suffix + '/'

    return group


def get_fit_workspace_directory(group_name, suffix, base_name, workspace_suffix):
    return base_name + '/' + group_name + workspace_suffix + '/' + group_name + suffix + workspace_suffix + '/'
