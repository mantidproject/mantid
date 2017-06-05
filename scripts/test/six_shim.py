from __future__ import (absolute_import, division, print_function)

import six

"""
This file provides compatibility with operating systems with older versions of Six. 
Currently this is for Ubuntu 14.04 which does not have Six 1.9.0 available"""


def assertRaisesRegex(self, *args, **kwargs):
    major, minor, patch = six.__version__.split(".")

    # Six version 1.9.0 onwards supports assertRaisesRegex
    if major == 1 and minor >= 9:
        return six.assertRaisesRegex(self, *args, **kwargs)

    # We are on older Six - manually handle the difference
    if six.PY2:
        return self.assertRaisesRegexp(*args, **kwargs)
    elif six.PY3:
        return self.assertRaisesRegex(*args, **kwargs)
    else:
        raise RuntimeError("Unknown Python version.")


def assertRegex(self, *args, **kwargs):
    major, minor, patch = six.__version__.split(".")

    # Six version 1.9.0 onwards supports assertRegex
    if major == 1 and minor >= 9:
        return six.assertRegex(self, *args, **kwargs)

    # We are on older Six - manually handle the difference
    if six.PY2:
        return self.assertRegexpMatches(*args, **kwargs)
    elif six.PY3:
        return self.assertRegex(*args, **kwargs)
    else:
        raise RuntimeError("Unknown Python version.")
