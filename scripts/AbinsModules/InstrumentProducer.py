from Instruments import ToscaInstrument, NoneInstrument
from AbinsModules import AbinsConstants

class InstrumentProducer(object):
    def __init__(self):
        pass

    def produceInstrument(self, name=None):

        if not name in AbinsConstants.all_instruments:
            raise ValueError("Unknown instrument: %s"%name)
        elif name == "TOSCA":
            return ToscaInstrument("TOSCA")
        elif name == "None":
            return NoneInstrument("None")


