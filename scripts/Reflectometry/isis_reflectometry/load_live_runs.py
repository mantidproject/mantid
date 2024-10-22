# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from mantid.api import mtd
from mantid.simpleapi import StartLiveData


def get_live_data(instrument_name, frequency=60, accumulation="Add", output_name="live"):
    StartLiveData(Instrument=str(instrument_name), UpdateEvery=frequency, Outputworkspace=str(output_name), AccumulationMethod=accumulation)
    ws = mtd[output_name]
    return ws


def is_live_run(run):
    try:
        return int(run) == 0
    except:
        return False
