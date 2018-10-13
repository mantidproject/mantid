# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

# 3rdparty imports
from qtpy import PYQT4
# local imports
from mantidqt.utils.qt import import_qt


if PYQT4:
    raise ImportError("Instrument view requires Qt >= v5")
# import widget class from C++ wrappers
InstrumentWidget = import_qt('._instrumentview', 'mantidqt.widgets.instrumentview',
                             'InstrumentWidget')
