# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class TransformSelectionPresenter(object):

    """
    The widget for selecting the widget
    shown in the transformation tab
    """

    def __init__(self, view):
        self.view = view

    @property
    def widget(self):
        return self.view

    def setMethodsCombo(self, options):
        self.view.setMethodsCombo(options)
