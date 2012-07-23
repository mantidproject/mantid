"""
    Defines a set of aliases for the kernel module to make
    accessing certain objects easier.
"""
from _kernel import ConfigServiceImpl, Logger, UnitFactoryImpl

###############################################################################
# Singletons - Make them just look like static classes
###############################################################################
ConfigService = ConfigServiceImpl.Instance()
config = ConfigService

UnitFactory = UnitFactoryImpl.Instance()

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger.get("Python")