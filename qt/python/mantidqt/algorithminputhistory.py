# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import)

from mantidqt.utils.qt import import_qt, toQSettings


_AlgorithmInputHistory = import_qt('._common', 'mantidqt', 'AlgorithmInputHistory')


class AlgorithmInputHistory(object):
    '''Wrapper class around MantidQtWidgets::Common::AlgorithmInputHistory
    '''
    _singleton = _AlgorithmInputHistory.Instance()

    def __init__(self):
        pass

    def readSettings(self, settings):
        self._singleton.readSettings(toQSettings(settings))

    def writeSettings(self, settings):
        self._singleton.writeSettings(toQSettings(settings))
