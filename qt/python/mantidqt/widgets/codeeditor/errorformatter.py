#  This file is part of the mantidqt package
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
