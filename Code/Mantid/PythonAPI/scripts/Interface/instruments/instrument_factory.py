"""
    Instrument interface factory.
    TODO: This module will responsible for the association between an instrument name
    and its corresponding interface class. 
"""
from hfir_interface import HFIRInterface

def instrument_factory(instrument_name):
    if str(instrument_name).strip().upper()=="BIOSANS":
        return HFIRInterface("BIOSANS")
        