"""Defines the Python interface to the Mantid framework
"""
import sys
import os

_moduledir = os.path.dirname(__file__)
# Ensure the sub modules can see each other
sys.path.append(_moduledir)

# If MANTIDPATH is not set, peek to see if a Mantid.properties file
# is in the parent directory
_bindir = os.path.dirname(_moduledir)
if not os.environ.has_key('MANTIDPATH') and os.path.exists(os.path.join(_bindir, 'Mantid.properties')):
    os.environ['MANTIDPATH'] = _bindir

# Mantid imports
from api import framework_mgr

# Start Mantid (mtd for old times sake)
mtd = framework_mgr