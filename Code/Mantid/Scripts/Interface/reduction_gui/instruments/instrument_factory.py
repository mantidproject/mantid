"""
    Instrument interface factory.
    This module is responsible for the association between an instrument name
    and its corresponding interface class. 
"""
from hfir_interface import HFIRInterface
from sns_interface import SNSInterface

INSTRUMENT_DICT = {"BIOSANS": HFIRInterface, 
                   "EQSANS": SNSInterface}

INSTRUMENT_LIST = INSTRUMENT_DICT.keys()

def instrument_factory(instrument_name, settings=None):
    for instrument in INSTRUMENT_DICT:
        if str(instrument_name).strip()==instrument:
            return INSTRUMENT_DICT[instrument](instrument, settings=settings)
        