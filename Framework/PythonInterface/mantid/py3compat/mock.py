# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import

try:
    # import for python 2 and python 3 with mock package installed
    from mock import *  # noqa
except ImportError:
    # Mock is within unittest.mock with Python 3
    from unittest.mock import *  # noqa
