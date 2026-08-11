# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantidqt.utils.qt import import_qt, toQSettings


_AlgorithmInputHistory = import_qt("._common", "mantidqt", "AlgorithmInputHistory")


class AlgorithmInputHistory(object):
    """Wrapper class around MantidQtWidgets::Common::AlgorithmInputHistory"""

    _singleton = _AlgorithmInputHistory.Instance()

    def __init__(self):
        pass

    def readSettings(self, settings):
        return self._singleton.readSettings(toQSettings(settings))

    def restoreSettings(self, settings):
        self._singleton.restoreSettings(settings)

    def captureSettings(self):
        return self._singleton.captureSettings()

    def saveSettings(self, settings, values):
        self._singleton.saveSettings(toQSettings(settings), values)
