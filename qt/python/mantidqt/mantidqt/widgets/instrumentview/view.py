# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
"""
Contains the Python wrapper class for the C++ instrument widget
"""

# 3rdparty imports
from qtpy.QtCore import Qt, Signal, Slot
from qtpy.QtWidgets import QVBoxLayout, QWidget

# local imports
from mantidqt.utils.qt import import_qt

# import widget class from C++ wrappers
from mantidqt.widgets.observers.observing_view import ObservingView

# _instrumentview.sip --> _instrumentview

InstrumentWidget = import_qt("._instrumentview", "mantidqt.widgets.instrumentview", "InstrumentWidget")


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

    def __init__(self, parent, presenter, name, window_flags=Qt.Window):
        super(InstrumentView, self).__init__(parent)

        self.widget = InstrumentWidget(name)

        # used by the observers view to delete the ADS observer
        self.presenter = presenter

        self.name = name

        self.setWindowTitle("Instrument - " + name)
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setWindowFlags(window_flags)

        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.widget)
        self.setLayout(layout)

        self.close_signal.connect(self._run_close)

    def get_tab(self, tab_index):
        tab_name = [InstrumentWidget.RENDER, InstrumentWidget.PICK, InstrumentWidget.MASK, InstrumentWidget.TREE][tab_index]
        print(f"Tab: {tab_name}")

        return self.widget.getTab(tab_name)

    def get_current_tab(self):
        """Get current tab
        :return: InstrumentWidgetTab
        """
        curr_index = self.widget.getCurrentTab()

        return self.get_tab(curr_index)

    def get_render_tab(self):
        return self.widget.getRenderTab(InstrumentWidget.RENDER)

    def get_pick_tab(self):
        return self.widget.getPickTab(InstrumentWidget.PICK)

    def select_tab(self, tab_index):
        self.widget.selectTab(tab_index)

    def set_range(self, min_value, max_value):
        self.widget.setBinRange(min_value, max_value)

    def replace_workspace(self, new_ws_name, new_window_name):
        if new_window_name is None:
            new_window_name = new_ws_name
        self.widget.replaceWorkspace(new_ws_name, new_window_name)

    def is_thread_running(self):
        return self.widget.isThreadRunning()

    def wait(self):
        return self.widget.waitForThread()

    def save_image(self, filename):
        return self.widget.saveImage(filename)

    def closeEvent(self, event):
        # ordering of close events is different depending on
        # whether workspace is deleted or window is closed
        if self.presenter is not None:
            # pass close event through to the underlying C++ widget
            children = self.findChildren(InstrumentWidget)
            for child in children:
                child.close()
            self.presenter.close(self.name)
        super(QWidget, self).closeEvent(event)

    @Slot()
    def _run_close(self):
        self.close()
