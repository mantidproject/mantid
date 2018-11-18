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

import atexit

from ._kernel import (ConfigServiceImpl, Logger, UnitFactoryImpl,
                      UsageServiceImpl, PropertyManagerDataServiceImpl)


def lazy_instance_access(cls, onexit=None):
    """
    Takes a singleton class and wraps it in an LazySingletonHolder
    that constructs the instance on first access.
    :param cls: The singleton class type
    :param onexit: Unbound method accepting a cls instance
    registered with atexit.register on instance construction
    :return: A new LazySingletonHolder wrapping cls
    """
    if onexit is not None:
        def instance(lazy_singleton):
            if object.__getattribute__(lazy_singleton,
                                       "atexit_not_registered"):
                atexit.register(lambda: onexit(cls.Instance()))
                object.__setattr__(lazy_singleton, "atexit_not_registered", False)
            return cls.Instance()
    else:
        def instance(_):
            return cls.Instance()

    class LazySingletonHolder(object):
        """
        Delays construction of a singleton instance until the
        first attribute access.
        """
        atexit_not_registered = True

        def __getattribute__(self, item):
            # Called for each attribute access. cls.Instance() constructs
            # the singleton at first access
            return cls.__getattribute__(instance(self), item)

        def __len__(self):
            return cls.__getattribute__(instance(self), "__len__")()

        def __getitem__(self, item):
            return cls.__getattribute__(instance(self), "__getitem__")(item)

        def __setitem__(self, item, value):
            return cls.__getattribute__(instance(self), "__setitem__")(item, value)

        def __delitem__(self, item):
            return cls.__getattribute__(instance(self), "__delitem__")(item)

        def __contains__(self, item):
            return cls.__getattribute__(instance(self), "__contains__")(item)

    return LazySingletonHolder()


# Historically the singleton aliases mapped to the instances rather than
# the class types, i.e. AnalysisDataService is the instance and not the type,
# which doesn't match the C++ behaviour.
UsageService = lazy_instance_access(UsageServiceImpl)
ConfigService = lazy_instance_access(ConfigServiceImpl)
PropertyManagerDataService = lazy_instance_access(PropertyManagerDataServiceImpl)
UnitFactory = lazy_instance_access(UnitFactoryImpl)

config = ConfigService
pmds = PropertyManagerDataService

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger("Python")
