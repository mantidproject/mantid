"""
    Instrument interface factory.
    This module is responsible for the association between an instrument name
    and its corresponding interface class.
"""
from reduction_gui.instruments.hfir_interface_dev import HFIRInterface
from reduction_gui.instruments.eqsans_interface_dev import EQSANSInterface
from reduction_gui.instruments.reflectometer_l_interface_dev import REFLInterface
from reduction_gui.instruments.reflectometer_m_interface_dev import REFMInterface
from reduction_gui.instruments.reflectometer_sf_interface_dev import REFLSFInterface
from reduction_gui.instruments.dgs_interface_dev import DgsInterface
from reduction_gui.instruments.diffraction_interface_dev import DiffractionInterface
from reduction_gui.instruments.toftof_interface_dev import TOFTOFInterface

INSTRUMENT_DICT = {"HFIR": {"BIOSANS": HFIRInterface,
                            "GPSANS": HFIRInterface},
                   "ISIS": {"MAPS": DgsInterface,
                            "MARI": DgsInterface,
                            "MERLIN": DgsInterface},
                   "MLZ":  {"TOFTOF": TOFTOFInterface},
                   "SNS":  {"ARCS": DgsInterface,
                            "CNCS": DgsInterface,
                            "EQSANS": EQSANSInterface,
                            "HYSPEC": DgsInterface,
                            "REFL": REFLInterface,
                            "REFLSF": REFLSFInterface,
                            "REFM": REFMInterface,
                            "SEQUOIA": DgsInterface,
                            "PG3": DiffractionInterface,
                            "NOM": DiffractionInterface,
                            "VULCAN": DiffractionInterface}
                   }


def instrument_factory(instrument_name, settings=None):
    for facility in INSTRUMENT_DICT:
        for instrument in INSTRUMENT_DICT[facility]:
            if str(instrument_name).strip()==instrument:
                return INSTRUMENT_DICT[facility][instrument](instrument, settings=settings)
