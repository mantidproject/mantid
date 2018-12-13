# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


def get_raw_data_workspace_name(context):
    return context._base_run_name() + "_raw_data"


def get_group_data_workspace_name(context, group_name):
    if context.is_multi_period():
        return context._base_run_name() + "; Group; " + group_name + \
               "; Counts; Periods; " + context.period_string + "; #1"
    else:
        return context._base_run_name() + "; Group; " + group_name + "; Counts; #1"


def get_pair_data_workspace_name(context, pair_name):
    if context.is_multi_period():
        return context._base_run_name() + "; Pair Asym; " + pair_name + "; Periods; " + context.period_string + "; #1"
    else:
        return context._base_run_name() + "; Pair Asym; " + pair_name + "; #1"


def get_base_data_directory(context):
    if context.is_multi_period():
        return context.base_directory + "/" + context._base_run_name() + " Period " + context.period_string + "/"
    else:
        return context.base_directory + "/" + context._base_run_name() + "/"


def get_raw_data_directory(context):
    if context.is_multi_period():
        return context._base_run_name() + " Period " + context.period_string + "; Raw Data/"
    else:
        return context._base_run_name() + " Raw Data/"


def get_cached_data_directory(context):
    if context.is_multi_period():
        return context._base_run_name() + " Period " + context.period_string + "; Cached/"
    else:
        return context._base_run_name() + " Cached/"


def get_group_data_directory(context):
    if context.is_multi_period():
        return context._base_run_name() + " Period " + context.period_string + "; Groups/"
    else:
        return context._base_run_name() + " Groups/"


def get_pair_data_directory(context):
    if context.is_multi_period():
        return context._base_run_name() + " Period " + context.period_string + "; Pairs/"
    else:
        return context._base_run_name() + " Pairs/"
