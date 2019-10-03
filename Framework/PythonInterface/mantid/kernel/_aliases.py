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

from mantid.kernel import (ConfigServiceImpl, Logger, PropertyManagerDataServiceImpl, UnitFactoryImpl, UsageServiceImpl)


def lazy_instance_access(cls):
    """
    Takes a singleton class and wraps it in an LazySingletonHolder
    that constructs the instance on first access.

     Historically, mantid <= v3.13, the singleton aliases mapped to the
    instances rather than the class types, i.e. UsageService is the instance and not the type.
    To preserve compatibility with existing scripts, where users have not called UsageService.Instance(),
    but also allow the singleton instance startup to be delayed we create a thin wrapper that
    delays the first .Instance() call until an attribute is accessed on the wrapper.


    :param cls: The singleton class type
    :return: A new LazySingletonHolder wrapping cls
    """

    class LazySingletonHolder(object):
        """
        Delays construction of a singleton instance until the
        first attribute access.
        """

        def __getattribute__(self, item):
            # Called for each attribute access. cls.Instance() constructs
            # the singleton at first access
            return cls.__getattribute__(cls.Instance(), item)

        def __len__(self):
            return cls.__getattribute__(cls.Instance(), "__len__")()

        def __getitem__(self, item):
            return cls.__getattribute__(cls.Instance(), "__getitem__")(item)

        def __setitem__(self, item, value):
            return cls.__getattribute__(cls.Instance(), "__setitem__")(item, value)

        def __delitem__(self, item):
            return cls.__getattribute__(cls.Instance(), "__delitem__")(item)

        def __contains__(self, item):
            return cls.__getattribute__(cls.Instance(), "__contains__")(item)

    return LazySingletonHolder()


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
