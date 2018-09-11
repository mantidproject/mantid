#  This file is part of the mantidqt package
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
