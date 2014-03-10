from mantid.simpleapi import *
def get_live_data(InstrumentName, frequency = 60, Accumulation = "Add", OutputName = "live"):
    StartLiveData(Instrument=str(InstrumentName), UpdateEvery = frequency, Outputworkspace=str(OutputName), AccumulationMethod = Accumulation)
    ws = mtd[OutputName]
    return ws
def is_live_run(run):
    try:
        return (int(run) is 0)
    except:
        return False
    