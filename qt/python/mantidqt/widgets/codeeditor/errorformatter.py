# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

# std imports
import traceback


class ErrorFormatter(object):
    """Formats errors to strings"""

    def format(self, exc_type, exc_value, stack):
        """
        Produce a formatted error message for the given
        error information.

        :param exc_type: The type of exception
        :param exc_value: An exception object of type exc_type
        :param stack: An optional stack trace (assumed to be part or
        all return by traceback.extract_tb
        :return: A formatted string.
        """
        lines = traceback.format_exception_only(exc_type, exc_value)
        if stack is not None:
            lines.extend(traceback.format_list(stack))
        return ''.join(lines)
