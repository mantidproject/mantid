# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as simple


def rebin_time(run, bin_param):
    wsname = _load_ws_to_process(run)
    output = "engg_preproc_time_ws"
    simple.Rebin(InputWorkspace=wsname, Params=bin_param, OutputWorkspace=output)
    return output


def rebin_pulse(run, bin_param): # , #n_periods): currently unused to match implementation in gui

    wsname = _load_ws_to_process(run)
    output = "engg_preproc_pulse_ws"
    simple.RebinByPulseTimes(InputWorkspace=wsname, Params=bin_param, OutputWorkspace=output)
    return output


def _load_ws_to_process(run):
    wsname = "engg_preproc_input_ws"
    simple.Load(Filename=run, OutputWorkspace=wsname)
    return wsname
