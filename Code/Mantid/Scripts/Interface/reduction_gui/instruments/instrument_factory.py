"""
    Instrument interface factory.
    This module is responsible for the association between an instrument name
    and its corresponding interface class. 
"""
from hfir_interface import HFIRInterface
from sns_interface import SNSInterface

INSTRUMENT_LIST = ["BIOSANS", "EQSANS"]

def instrument_factory(instrument_name, settings=None):
    if str(instrument_name).strip().upper()=="BIOSANS":
        return HFIRInterface("BIOSANS", settings=settings)
    elif str(instrument_name).strip().upper()=="EQSANS":
        return SNSInterface("EQSANS", settings=settings)
        