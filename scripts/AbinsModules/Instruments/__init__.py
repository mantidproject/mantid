# flake8: noqa
from __future__ import (absolute_import, division, print_function)
# Make sure we can find Instruments...
import mantid
import os
import sys
one_path = mantid.config["pythonscripts.directories"].split(";")[0]
instrument_path = os.path.join(one_path[:one_path.index("scripts")], "scripts", "AbinsModules", "Instruments")
sys.path.append(instrument_path)

from .ToscaInstrument import ToscaInstrument
from .TwoDMap import TwoDMap
from .Instrument import Instrument
