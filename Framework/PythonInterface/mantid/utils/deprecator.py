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


def deprecated_algorithm(new_name, deprecation_date):  # decorator factory
    r"""
    :brief: Class decorator marking the algorithm as deprecated

    :param str new_name: Algorithm's name replacing the deprecated algorithm. If there's no replacement, pass
        and empty string or ``None``.
    :param str deprecation_date:  date of deprecation for this algorithm, in ISO8601 format
    """
    try:
        parse_date(deprecation_date)
    except ValueError:
        logger.error(f'Algorithm deprecation date {deprecation_date} must be in ISO8601 format')

    def decorator_instance(cls):
        depr_msg = f'Algorithm "{cls.__name__}" is deprecated since {deprecation_date}.'
        if new_name:
            depr_msg += ' Use "{new_name}" instead'
        raise_msg = f'Configuration "algorithms.deprecated" set to raise upon use of any deprecated algorithm'

        cls_category = cls.category
        @functools.wraps(cls_category)
        def new_category(self):
            r"""Add the Deprecated category"""
            return cls_category(self) + ';Deprecated'
        cls.category = new_category

        cls_PyExec = cls.PyExec
        @functools.wraps(cls_PyExec)
        def new_PyExec(self):
            r"""decorate PyExec to raise or log an error message"""
            if ConfigService.Instance()['algorithms.deprecated'].lower() == 'raise':
                raise RuntimeError(raise_msg)
            cls_PyExec(self)
            logger.error(depr_msg)  # show error after execution
        cls.PyExec = new_PyExec
        return cls
    return decorator_instance
