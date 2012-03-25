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

###############################################################################
# Define the api version
###############################################################################
def apiVersion():
    """Indicates that this is version 2
    of the API
    """
    return 2

###############################################################################
# GUI - Do this as early as possible
###############################################################################
# Flag indicating whether the GUI layer is loaded. 
try:
    import _qti
    __gui__ = True
except ImportError:
    __gui__ = False

###############################################################################
# Try to be smarter when finding Mantid framework libraries
###############################################################################
# Peek to see if a Mantid.properties file is in the parent directory,
# if so assume that it is the required Mantid bin directory containing
# the Mantid libraries and ignore any MANTIDPATH that has been set
import os as _os
_moduledir = _os.path.abspath(_os.path.dirname(__file__))
_bindir = _os.path.dirname(_moduledir)
if _os.path.exists(_os.path.join(_bindir, 'Mantid.properties')):
    _os.environ['MANTIDPATH'] = _bindir

###############################################################################
# Make most things accessible from mantid namespace 
###############################################################################
import kernel
from kernel import *

import geometry
from geometry import *

import api 
from api import *

###############################################################################
# Make the version string accessible in the standard way
###############################################################################
__version__ = version_str()

