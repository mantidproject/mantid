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
# Set deprecation warnings back to default (they are ignored in 2.7)
###############################################################################
import warnings as _warnings
_warnings.filterwarnings("default",category=DeprecationWarning)

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

def _to_set(key):
    s = set(kernel.config[key].split(";"))
    if '' in s:
        s.remove('')
    return s

_plugins_key = 'python.plugins.directories'
_user_key = 'user.%s' % _plugins_key
plugin_dirs = _to_set(_plugins_key)
plugin_dirs.update(_to_set(_user_key))

# Check deprecated path too
# There are 2 situations that require handling:
#   2) Users defined new algorithms and placed them in other locations and added these
#      locations to the pythonalgorithms.directories key in their user.props file. Tell them to update the
#      and use the new keys
#   2) Users defined new algorithms and placed them in the deprecated location so the key is not in their 
#      user.properties file. Ask them to move the files to the new location
_deprecated_key = 'pythonalgorithms.directories'
_old_locs_key = '%s.deprecated' % (_deprecated_key)
# 1)
_user_file = config.getUserFilename()
_deprecated_locs = _to_set(_deprecated_key)
_msg="The Python algorithms key '%s' in '%s' has been deprecated. Please add '%s' to the '%s' key instead. " +\
     "Future release will not check the old key."
for loc in _deprecated_locs:
    loc = loc.rstrip("/")
    if (not loc.endswith('PythonAPI/PythonAlgorithms')) and (not loc.endswith('PythonInterface/PythonAlgorithms')): # Avoid dev warning
        logger.warning(_msg % (_deprecated_key,_user_file, loc,_user_key))
        plugin_dirs.add(loc)

# 2)
_old_locs = _to_set(_old_locs_key)
_new_loc = _os.path.abspath(_os.path.join(_os.path.dirname(_bindir), '../plugins/python/algorithms'))
_msg="The packaged Python algorithms have been moved. You have extra algorithms in '%s', please move these files to '%s'. " +\
     "Future releases will not check this location."
for loc in _old_locs:
    loc = loc.rstrip("/")
    if _plugins.check_for_plugins(loc):
        if 'PythonAPI/PythonAlgorithms' not in loc: # Avoid a warning for developers until all old algorithms are gone
            logger.warning(_msg % (loc,_new_loc))
        plugin_dirs.add(loc)

# Load
plugin_files = []
alg_files = []
for directory in plugin_dirs:
    try:
        all_plugins, algs = _plugins.find_plugins(directory)
        plugin_files += all_plugins
        alg_files += algs
    except ValueError, exc:
        logger.warning(str(exc))
        continue

# Mockup the full API first so that any Python algorithm module has something to import
_simpleapi._mockup(alg_files)
# Load the plugins
plugin_modules = _plugins.load(plugin_files)
# Create the proper algorithm definitions in the module
new_attrs = _simpleapi._translate()
# Finally, overwrite the mocked function definitions in the loaded modules with the real ones 
_plugins.sync_attrs(_simpleapi, new_attrs, plugin_modules)

################################################################################
