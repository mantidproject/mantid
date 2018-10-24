# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
"""
Contains the presenter for displaying the InstrumentWidget
"""
from __future__ import (absolute_import, unicode_literals)

# local imports
from .view import InstrumentView


class InstrumentViewPresenter(object):
    """
    Presenter holding the view widget for the InstrumentView.
    It has no model as its an old widget written in C++ with out MVP
    """
    view = None

    def __init__(self, ws, parent=None):
        self.view = InstrumentView(self, ws.name(), parent)
