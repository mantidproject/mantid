"""
    Instrument interface factory.
    This module is responsible for the association between an instrument name
    and its corresponding interface class. 
"""
#from hfir_interface import HFIRInterface as HFIRInterfaceOld
#from eqsans_interface import EQSANSInterface
from hfir_interface_dev import HFIRInterface
from eqsans_interface_dev import EQSANSInterface


INSTRUMENT_DICT = {"HFIR": {"BIOSANS": HFIRInterface, 
                            "GPSANS": HFIRInterface,
                            "HFIRDEV": HFIRInterface},
                   "SNS":  {"EQSANS": EQSANSInterface,
                            "EQDEV": EQSANSInterface}}               

def instrument_factory(instrument_name, settings=None):
    for facility in INSTRUMENT_DICT:
        for instrument in INSTRUMENT_DICT[facility]:
            if str(instrument_name).strip()==instrument:
                return INSTRUMENT_DICT[facility][instrument](instrument, settings=settings)
