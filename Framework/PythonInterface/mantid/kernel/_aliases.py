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


def create_instance_holder(cls):
    """
    Takes a singleton class and wraps it in an SingletonHolder
    that constructs the instance on first access.
    :param cls: The singleton class type
    :return: A new SingletonHolder wrapping cls
    """
    class SingletonHolder(object):
        """
        Delays construction of a singleton instance until the
        first attribute access.
        """
        def __getattribute__(self, item):
            # Called for each attribute access. cls.Instance()
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

    return SingletonHolder()


# Historically the singleton aliases mapped to the instances rather than
# the class types, i.e. AnalysisDataService is the instance and not the type,
# which doesn't match the C++ behaviour.
UsageService = create_instance_holder(UsageServiceImpl)
ConfigService = create_instance_holder(ConfigServiceImpl)
PropertyManagerDataService = create_instance_holder(PropertyManagerDataServiceImpl)
UnitFactory = create_instance_holder(UnitFactoryImpl)

config = ConfigService
pmds = PropertyManagerDataService

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger("Python")
