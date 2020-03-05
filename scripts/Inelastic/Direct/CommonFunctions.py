# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function, unicode_literals)

import sys

# See https://www.python.org/dev/peps/pep-0479/#abstract and
# https://stackoverflow.com/a/51701040
if sys.version_info >= (3,7):
    NEW_STYLE_GENERATOR = True
else:
    NEW_STYLE_GENERATOR = False


class switch(object):
    """ Helper class providing nice switch statement"""

    def __init__(self, value):
        self.value = value
        self.fall = False

    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
        if NEW_STYLE_GENERATOR:
            return
        else:
            raise StopIteration

    def match(self, *args):
        """Indicate whether or not to enter a case suite"""
        if self.fall or not args:
            return True
        elif self.value in args: # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False
