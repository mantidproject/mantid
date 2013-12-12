from mantid.simpleapi import *
def LoadData(runNumber, InstrumentName)
    try:
        if zeroCheck(runNumber):
            StartLiveData(Instrument=str(InstrumentName), Outputworkspace=str(runNumber))
        else:
            currentInstrument = config['default.instrument']
            config['default.instrument'] = InstrumentName
            Load(Filename=str(runNumber),Outputworkspace=str(runNumber))
            config['default.instrument'] = currentInstrument
        ws = mtd[runNumber]
        return ws
    except:
        raise
def zeroCheck(run):
    try:
        return (int(run) is 0)
    except:
        raise ValueError("Run numbers must be integer values")
    