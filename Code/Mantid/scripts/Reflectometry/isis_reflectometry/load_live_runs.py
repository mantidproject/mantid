#pylint: disable=invalid-name
from mantid.simpleapi import *
def get_live_data(instrument_name, frequency = 60, accumulation = "Add", output_name = "live"):
    StartLiveData(Instrument=str(instrument_name), UpdateEvery = frequency, Outputworkspace=str(output_name), AccumulationMethod = accumulation)
    ws = mtd[output_name]
    return ws
def is_live_run(run):
    try:
        return (int(run) is 0)
    except:
        return False
