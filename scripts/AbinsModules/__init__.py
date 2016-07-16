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
from CalculateDW import CalculateDW
from CalculateMSD import CalculateMSD

# Data
from GeneralData import GeneralData
from QData import  QData
from KpointsData import KpointsData
from AtomsData import AtomsDaTa
from AbinsData import  AbinsData
from DwData import  DwData
from MSDData import MSDData

# Instruments
from InstrumentProducer import  InstrumentProducer