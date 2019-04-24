# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


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
