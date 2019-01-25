# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (print_function, absolute_import, unicode_literals)

from mantidqt.utils.qt import import_qt


BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'IndirectFitPropertyBrowser')


class IndirectFitPropertyBrowser(BaseBrowser):

    def __init__(self, parent=None):
        super(IndirectFitPropertyBrowser, self).__init__(parent)
        self.init()
