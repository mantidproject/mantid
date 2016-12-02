from Instruments import ToscaInstrument, TwoDMap
from AbinsModules import AbinsConstants


class InstrumentProducer(object):
    def __init__(self):
        pass

    def produce_instrument(self, name=None):

        if name not in AbinsConstants.ALL_INSTRUMENTS:
            raise ValueError("Unknown instrument: %s" % name)
        elif name == "TOSCA":
            return ToscaInstrument("TOSCA")
        elif name == "TwoDMap":
            return TwoDMap("TwoDMap")
