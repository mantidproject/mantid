"""
Mantid
======

http://www.mantidproject.org

The Mantid project provides a platform that supports high-performance computing
on neutron and muon data. The framework provides a set of common services,
algorithms and data objects that are:

    - Instrument or technique independent;
    - Supported on multiple target platforms (Windows, Linux, Mac OS X);
    - Easily extensible by Instruments Scientists/Users;
    - Open source and freely redistributable to visiting scientists;
    - Provides functionalities for Scripting, Visualization, Data transformation,
      Implementing Algorithms, Virtual Instrument Geometry.

"""
# flake8: noqa
from __future__ import (absolute_import, division, print_function)

# Make sure we can find AbinsModules...
import mantid
import os
import sys
one_path = mantid.config["pythonscripts.directories"].split(";")[0]
abins_path = os.path.join(one_path[:one_path.index("scripts")], "scripts", "AbinsModules")
sys.path.append(abins_path)

from .IOmodule import IOmodule

# Frequency generator
from .FrequencyPowderGenerator import FrequencyPowderGenerator

# Loading modules
from .GeneralDFTProgram import GeneralDFTProgram
from .LoadCASTEP import LoadCASTEP
from .LoadCRYSTAL import LoadCRYSTAL

# Calculating modules
from .CalculatePowder import CalculatePowder
from .CalculateSingleCrystal import CalculateSingleCrystal
from .CalculateDWSingleCrystal import CalculateDWSingleCrystal
from .CalculateS import CalculateS

# Data
from .GeneralData import GeneralData
from .KpointsData import KpointsData
from .AtomsData import AtomsDaTa
from .AbinsData import AbinsData
from .DWSingleCrystalData import DWSingleCrystalData
from .SingleCrystalData import SingleCrystalData
from .PowderData import PowderData
from .SData import SData

# Instruments
from .InstrumentProducer import InstrumentProducer

from . import AbinsParameters

