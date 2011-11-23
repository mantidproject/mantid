"""Defines the Python interface to the Mantid framework
"""
import sys
import os

########################################################
# Path settings
########################################################
# Ensure the sub modules can see each other
_moduledir = os.path.abspath(os.path.dirname(__file__))
sys.path.append(_moduledir)

# Peek to see if a Mantid.properties file is in the parent directory,
# if so assume that it is the required Mantid bin directory containing
# the Mantid libraries and ignore any MANTIDPATH that has been set
_bindir = os.path.dirname(_moduledir)
if os.path.exists(os.path.join(_bindir, 'Mantid.properties')):
    os.environ['MANTIDPATH'] = _bindir

########################################################
# Mantid imports
########################################################
from api import framework_mgr

# Start Mantid (mtd for old times sake)
mtd = framework_mgr
