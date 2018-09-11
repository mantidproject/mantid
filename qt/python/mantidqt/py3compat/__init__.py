#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
mantidqt.py3compat
------------------

Transitional module providing compatibility functions intended to help
migrating from Python 2 to Python 3. Mostly just wraps six but allowing
for additional functionality of our own.

This module should be fully compatible with:
    * Python >=v2.7
    * Python 3
"""

from __future__ import (absolute_import, print_function, unicode_literals)

import six
from six import *  # noqa

# Enumerations are built in with Python 3
try:
    from enum import Enum
except ImportError:
    # use a compatability layer
    from mantidqt.py3compat.enum import Enum  # noqa

# -----------------------------------------------------------------------------
# Globals and constants
# -----------------------------------------------------------------------------
__all__ = dir(six)


# -----------------------------------------------------------------------------
# Strings
# -----------------------------------------------------------------------------
def is_text_string(obj):
    """Return True if `obj` is a text string, False if it is anything else,
    like binary data (Python 3) or QString (Python 2, PyQt API #1)"""
    return isinstance(obj, string_types)


def to_text_string(obj, encoding=None):
    """Convert `obj` to (unicode) text string"""

    if PY2:
        # Python 2
        if encoding is None:
            return unicode(obj)
        else:
            return unicode(obj, encoding)
    else:
        # Python 3
        if encoding is None:
            return str(obj)
        elif isinstance(obj, str):
            # In case this function is not used properly, this could happen
            return obj
        else:
            return str(obj, encoding)


def qbytearray_to_str(qba):
    """Convert QByteArray object to str in a way compatible with Python 2/3"""
    return str(bytes(qba.toHex().data()).decode())
