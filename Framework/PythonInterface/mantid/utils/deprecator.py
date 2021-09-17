# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.kernel import ConfigService, logger

# third-party imports
from dateutil.parser import parse as parse_date

# standard imports
import functools


def deprecated_alias(deprecation_date):  # decorator factory
    r"""
    :brief: Class decorator marking the algorithm's alias as deprecated

    :details: It's assumed that the algorithm implements the alias method

    :param str deprecation_date: date of deprecation for the alias, in ISO8601 format
    """
    try:
        parse_date(deprecation_date)
    except ValueError:
        logger.error(f'Alias deprecation date {deprecation_date} must be in ISO8601 format')

    def decorator_instance(cls):
        cls_aliasDeprecated = cls.aliasDeprecated
        @functools.wraps(cls_aliasDeprecated)
        def new_aliasDeprecated(self):
            return deprecation_date
        cls.aliasDeprecated = new_aliasDeprecated
        return cls

    return decorator_instance
