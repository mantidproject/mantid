# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
mantid.py3compat
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
import sys

# Enumerations are built in with Python 3
try:
    from enum import Enum
except ImportError:
    # use a compatability layer
    from mantid.py3compat.enum import Enum  # noqa

# -----------------------------------------------------------------------------
# Globals and constants
# -----------------------------------------------------------------------------
__all__ = dir(six)


# -----------------------------------------------------------------------------
# System functions
# -----------------------------------------------------------------------------
if six.PY2 or sys.version_info[0:2] < (3, 2):
    setswitchinterval = sys.setcheckinterval
else:
    setswitchinterval = sys.setswitchinterval


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
