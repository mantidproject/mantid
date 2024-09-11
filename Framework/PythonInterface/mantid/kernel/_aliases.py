# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Defines a set of aliases for the kernel module to make
accessing certain objects easier.
"""

from mantid.kernel import ConfigServiceImpl, Logger, PropertyManagerDataServiceImpl, UnitFactoryImpl, UsageServiceImpl


def lazy_instance_access(cls, key_as_str=False):
    """
    Takes a singleton class and wraps it in an LazySingletonHolder
    that constructs the instance on first access.

     Historically, mantid <= v3.13, the singleton aliases mapped to the
    instances rather than the class types, i.e. UsageService is the instance and not the type.
    To preserve compatibility with existing scripts, where users have not called UsageService.Instance(),
    but also allow the singleton instance startup to be delayed we create a thin wrapper that
    delays the first .Instance() call until an attribute is accessed on the wrapper.


    :param cls: The singleton class type
    :param key_as_str: When ``True``, the key supplied to ``__getitem__``, ``__setitem__``,
    ``__delitem__``, and ``__contains__`` will be converted to a str. ``None`` wil return ``False``
    for ``__contains__``
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
            if key_as_str:
                if item is None:
                    raise KeyError(item)
                else:
                    cls.__getattribute__(cls.Instance(), "__getitem__")(str(item))
            return cls.__getattribute__(cls.Instance(), "__getitem__")(item)

        def __setitem__(self, item, value):
            if key_as_str:
                if item is None:
                    raise KeyError(item)
                else:
                    return cls.__getattribute__(cls.Instance(), "__setitem__")(str(item), value)
            return cls.__getattribute__(cls.Instance(), "__setitem__")(item, value)

        def __delitem__(self, item):
            if key_as_str:
                if item is None:
                    raise KeyError(item)
                else:
                    return cls.__getattribute__(cls.Instance(), "__delitem__")(str(item))
            return cls.__getattribute__(cls.Instance(), "__delitem__")(item)

        def __contains__(self, item):
            if key_as_str:
                # check for things that evaluate to False
                if (item is None) or (not bool(item)):
                    return False
                else:
                    # convert the ``item`` to a string
                    return cls.__getattribute__(cls.Instance(), "__contains__")(str(item))
            else:
                return cls.__getattribute__(cls.Instance(), "__contains__")(item)

    return LazySingletonHolder()


UsageService = lazy_instance_access(UsageServiceImpl)
ConfigService = lazy_instance_access(ConfigServiceImpl, key_as_str=True)
PropertyManagerDataService = lazy_instance_access(PropertyManagerDataServiceImpl, key_as_str=True)
UnitFactory = lazy_instance_access(UnitFactoryImpl)

config = ConfigService
pmds = PropertyManagerDataService

###############################################################################
# Set up a general Python logger. Others can be created as they are required
# if a user wishes to be more specific
###############################################################################
logger = Logger("Python")
