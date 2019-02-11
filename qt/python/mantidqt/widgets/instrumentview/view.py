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
Contains the Python wrapper class for the C++ instrument widget
"""
from __future__ import (absolute_import, unicode_literals)

# 3rdparty imports
from qtpy.QtCore import Qt, Signal, Slot
from qtpy.QtWidgets import QVBoxLayout, QWidget

# local imports
from mantidqt.utils.qt import import_qt
# import widget class from C++ wrappers
from mantidqt.widgets.observers.observing_view import ObservingView

InstrumentWidget = import_qt('._instrumentview', 'mantidqt.widgets.instrumentview',
                             'InstrumentWidget')


class InstrumentView(QWidget, ObservingView):
    """
    Defines a Window wrapper for the instrument widget. Sets
    the Qt.Window flag and window title. Holds a reference
    to the presenter and keeps it alive for the duration that
    the window is open
    """
    _presenter = None
    _widget = None

    close_signal = Signal()

    def __init__(self, parent, presenter, name):
        super(InstrumentView, self).__init__(parent)

        self.widget = InstrumentWidget(name)

        # used by the observers view to delete the ADS observer
        self.presenter = presenter

        self.setWindowTitle(name)
        self.setWindowFlags(Qt.Window)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.close_signal.connect(self._run_close)

    @Slot()
    def _run_close(self):
        self.close()
