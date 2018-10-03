# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
    Defines a set of aliases for the kernel module to make
    accessing certain objects easier.
"""
from __future__ import (absolute_import, division,
                        print_function)

from ._kernel import (ConfigServiceImpl, Logger, UnitFactoryImpl,
                      UsageServiceImpl, PropertyManagerDataServiceImpl)

###############################################################################
# Singletons - Make them just look like static classes
###############################################################################
UsageService = UsageServiceImpl.Instance()
ConfigService = ConfigServiceImpl.Instance()
config = ConfigService

PropertyManagerDataService = PropertyManagerDataServiceImpl.Instance()
pmds = PropertyManagerDataService

UnitFactory = UnitFactoryImpl.Instance()

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger("Python")
