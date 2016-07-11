from Instruments import ToscaInstrument
from AbinsModules import Constants

class InstrumentProducer(object):
    def __init__(self):
        pass

    def produceInstrument(self, name=None):

        if not name in Constants.all_instruments:
            raise ValueError("Inknown instrument: %s"%name)
        elif name == "TOSCA":
            return ToscaInstrument("TOSCA")

