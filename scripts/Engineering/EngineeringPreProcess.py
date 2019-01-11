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
    simple.Rebin(InputWorkspace=wsname, Params=bin_param, OutputWorkspace="engg_preproc_time_ws")


def rebin_pulse(run, bin_param, n_periods):
    _n_periods = n_periods  # currently unused to match implementation in gui
    wsname = _load_ws_to_process(run)
    simple.RebinByPulseTimes(InputWorkspace=wsname, Params=bin_param, OutputWorkspace="engg_preproc_pulse_ws")


def _load_ws_to_process(run):
    wsname = "engg_preproc_input_ws"
    simple.Load(Filename=run, OutputWorkspace=wsname)
    return wsname
