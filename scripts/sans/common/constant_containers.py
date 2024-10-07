# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans.common.constants import LARMOR, LOQ, SANS2D, ZOOM
from sans.common.enums import SANSInstrument

# ----------------------------------------------------------------------------------------------------------------------
# SANSInstrument
# ----------------------------------------------------------------------------------------------------------------------
SANSInstrument_string_as_key = {
    LARMOR: SANSInstrument.LARMOR,
    LOQ: SANSInstrument.LOQ,
    SANS2D: SANSInstrument.SANS2D,
    ZOOM: SANSInstrument.ZOOM,
}

# Include NoInstrument in a dict
SANSInstrument_string_as_key_NoInstrument = SANSInstrument_string_as_key.copy()
SANSInstrument_string_as_key_NoInstrument.update({"NoInstrument": SANSInstrument.NO_INSTRUMENT})
SANSInstrument_string_as_key_NoInstrument.update({"No Instrument": SANSInstrument.NO_INSTRUMENT})

SANSInstrument_enum_as_key = {
    SANSInstrument.LARMOR: LARMOR,
    SANSInstrument.LOQ: LOQ,
    SANSInstrument.SANS2D: SANS2D,
    SANSInstrument.ZOOM: ZOOM,
}

SANSInstrument_enum_as_key_NoInstrument = SANSInstrument_enum_as_key.copy()
SANSInstrument_enum_as_key_NoInstrument.update({SANSInstrument.NO_INSTRUMENT: "NoInstrument"})

SANSInstrument_string_list = [LARMOR, LOQ, SANS2D, ZOOM]
SANSInstrument_enum_list = [SANSInstrument.LARMOR, SANSInstrument.LOQ, SANSInstrument.SANS2D, SANSInstrument.ZOOM]
