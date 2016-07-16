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



from IOmodule import  IOmodule

# Calculating modules
from GeneralDFTProgram import GeneralDFTProgram
from LoadCASTEP import LoadCASTEP
from CalculateQ import CalculateQ
from CalculateDWCrystal import CalculateDWCrystal
from CalculatePowder import CalculatePowder

# Data
from GeneralData import GeneralData
from QData import  QData
from KpointsData import KpointsData
from AtomsData import AtomsDaTa
from AbinsData import  AbinsData
from DwCrystalData import  DwCrystalData
from PowderData import PowderData

# Instruments
from InstrumentProducer import  InstrumentProducer