"""
    Defines a set of aliases for the kernel module to make
    accessing certain objects easier.
"""
from _kernel import ConfigServiceImpl, Logger

###############################################################################
# Singletons
###############################################################################
ConfigService = ConfigServiceImpl.Instance()
config = ConfigService

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger.get("Python")