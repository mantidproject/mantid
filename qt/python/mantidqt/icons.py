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
Icon access. Icons are provided by qtawesome.
"""
from __future__ import absolute_import

# std imports

# third party imports
import qtawesome as qta


def get_icon(*names, **kwargs):
    """Return a QIcon corresponding to the icon name.
    See qtawesome.icon documentation for a full list of options.
    """
    return qta.icon(*names, **kwargs)
