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

from mantidqt.utils.qt import import_qt


_AlgorithmInputHistory = import_qt('._common', 'mantidqt', 'AlgorithmInputHistory')


def _toQSettings(settings):
    '''Utility function to convert supplied settings object to a qtpy.QtCore.QSettings
    '''
    try: # workbench.config.user
        return settings.qsettings
    except: # must be a QSettings already
        return settings


class AlgorithmInputHistory(object):
    '''Wrapper class around MantidQtWidgets::Common::AlgorithmInputHistory
    '''
    _singleton = _AlgorithmInputHistory.Instance()

    def __init__(self):
        pass

    def readSettings(self, settings):
        self._singleton.readSettings(_toQSettings(settings))

    def writeSettings(self, settings):
        self._singleton.writeSettings(_toQSettings(settings))
