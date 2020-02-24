# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy import uic
from qtpy.QtWidgets import QWidget
from ..base.view import BaseView
import os


class ReflTabView(BaseView, QWidget):

    def __init__(self):
        BaseView.__init__(self)
        here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(here, 'view.ui'), self)
        self.show()