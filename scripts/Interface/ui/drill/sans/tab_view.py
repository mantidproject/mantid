# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from .ui_view import Ui_SansTabs
from ..base.view import BaseView


class SansTabView(BaseView, Ui_SansTabs):

    def __init__(self):
        BaseView.__init__(self)
        Ui_SansTabs.__init__(self)
        self.setupUi(self)