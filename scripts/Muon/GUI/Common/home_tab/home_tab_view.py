from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore


class HomeTabView(QtGui.QWidget):

    def __init__(self, parent=None,
                 widget_list=None
                 # instrument_widget=None,
                 # grouping_widget=None,
                 # plot_widget=None,
                 # run_info_widget=None
                 ):
        super(HomeTabView, self).__init__(parent)

        self._widget_list = widget_list

        # self._instrument_widget = instrument_widget
        # self._grouping_widget = grouping_widget
        # self._plot_widget = plot_widget
        # self._run_info_widget = run_info_widget

        self.splitter = None
        self.vertical_layout = None

        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("HomeTab")
        self.setWindowTitle("Home Tab")
        self.resize(500, 100)

        self.splitter = QtGui.QSplitter(QtCore.Qt.Vertical)

        self.vertical_layout = QtGui.QVBoxLayout()

        if self._widget_list:
            for i, widget in enumerate(self._widget_list):
                self.splitter.addWidget(widget)
                self.splitter.setCollapsible(i, False)

        # self.splitter.addWidget(self._instrument_widget)
        # self.splitter.addWidget(self._grouping_widget)
        # self.splitter.addWidget(self._plot_widget)
        # self.splitter.addWidget(self._run_info_widget)
        # self.splitter.setCollapsible(0, False)
        # self.splitter.setCollapsible(1, False)
        # self.splitter.setCollapsible(2, False)
        # self.splitter.setCollapsible(3, False)
        self.splitter.setHandleWidth(2)
        self.setStyleSheet("QSplitter::handle {background-color: #3498DB}")

        self.vertical_layout.addWidget(self.splitter)
        self.setLayout(self.vertical_layout)

    # for docking
    def getLayout(self):
        return self.vertical_layout
