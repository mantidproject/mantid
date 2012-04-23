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
# Ensure the sub package C libraries are loaded
###############################################################################
import kernel
import geometry
import api 

###############################################################################
# Make the aliases form each module accessible in a the mantid namspace
###############################################################################
from kernel._aliases import *
from api._aliases import *

###############################################################################
# Make the version string accessible in the standard way
###############################################################################
__version__ = kernel.version_str()

###############################################################################
# Load the Python plugins now everything has started.
#
# Before the plugins are loaded the simpleapi module is called to create
# fake error-raising functions for all of the plugins. After the plugins have been 
# loaded the correction translation is applied to create the "real" simple
# API functions.
#
# Although this seems odd it is necessary so that any PythonAlgorithm 
# can call any other PythonAlgorithm through the simple API mechanism. If left 
# to the simple import mechanism then plugins that are loaded later cannot
# be seen by the earlier ones (chicken & the egg essentially). 
################################################################################
import simpleapi as _simpleapi
from kernel import plugins as _plugins

#_simpleapi.mockout_api()
_plugins.load(kernel.config['pythonalgorithms.directories'])
# Now everything is loaded create the proper definitions
_simpleapi.translate()
