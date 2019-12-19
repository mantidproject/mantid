# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
import Muon.GUI.Common.message_box as message_box
from matplotlib.figure import Figure
from mantidqt.plotting.functions import get_plot_fig
from matplotlib.backends.qt_compat import is_pyqt5
from MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import (
        FigureCanvas, NavigationToolbar2QT as NavigationToolbar)
else:
    from matplotlib.backends.backend_qt4agg import (
        FigureCanvas, NavigationToolbar2QT as NavigationToolbar)


class PlotWidgetView(QtWidgets.QWidget):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(PlotWidgetView, self).__init__(parent)
        self.plot_label = None
        self.plot_selector = None
        self.tile_type_selector = None
        self.tiled_type_label = None
        self.plot_type_selector = None
        self.group_selector = None
        self.horizontal_layout = None
        self.vertical_layout = None
        self.raw = None
        self.fig = None
        self.toolBar = None
        self.group = None
        self.widget_layout = None
        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("PlotWidget")
        self.resize(500, 100)

        self.plot_label = QtWidgets.QLabel(self)
        self.plot_label.setObjectName("plotLabel")
        self.plot_label.setText("Plot type : ")

        self.tiled_type_label = QtWidgets.QLabel(self)
        self.tiled_type_label.setObjectName("tileTypeLabel")
        self.tiled_type_label.setText("Tile plots by: ")

        self.raw = QtWidgets.QCheckBox("plot raw", self)
        self.raw.setChecked(True)

        self.plot_selector = QtWidgets.QComboBox(self)
        self.plot_selector.setObjectName("plotSelector")
        self.plot_selector.addItems(["Asymmetry", "Counts"])
        self.plot_selector.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
                                         QtWidgets.QSizePolicy.Fixed)

        self.plot_type_selector = QtWidgets.QCheckBox(parent=self)
        self.plot_type_selector.setChecked(False)
        self.plot_type_selector.setObjectName("plotTypeSelector")
        self.plot_type_selector.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
                                              QtWidgets.QSizePolicy.Fixed)

        self.tile_type_selector = QtWidgets.QComboBox(self)
        self.tile_type_selector.setObjectName("tileTypeSelector")
        self.tile_type_selector.addItems(["Group/Pair", "Run"])

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.plot_label)
        self.horizontal_layout.addWidget(self.plot_selector)
        self.horizontal_layout.addSpacing(15)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.plot_type_selector)
        self.horizontal_layout.addWidget(self.tiled_type_label)
        self.horizontal_layout.addWidget(self.tile_type_selector)
        self.horizontal_layout.addSpacing(15)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.raw)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addSpacing(50)

        # plotting options
        self.plot_options = QuickEditWidget(self)
        self.plot_options.widget.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)

        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)

        self.group = QtWidgets.QGroupBox("Plotting")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                           "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: 0px;'
                           'padding-bottom: 0px;'
                           "padding-right: 10px;"
                           ' color: grey; }')

        self.group.setLayout(self.vertical_layout)

        # create the figure
        self.fig = Figure()
        self.fig.canvas = FigureCanvas(self.fig)
        self.toolBar = NavigationToolbar(self.fig.canvas, self)
        # set size policy
        self.toolBar.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)

        # Create a set of Mantid axis for the figure
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=1,
                                      fig=self.fig)

        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.vertical_layout.addWidget(self.toolBar)
        self.vertical_layout.addWidget(self.fig.canvas)

        self.vertical_layout.addWidget(self.plot_options.widget)

        self.setLayout(self.widget_layout)

    # for docking
    def getLayout(self):
        return self.widget_layout

    # this ensures the plot layout changes when the size of the plot widget window is changed by the user
    def resizeEvent(self, QResizeEvent):
        self.fig.tight_layout()

    def if_overlay(self):
        return self.overlay.isChecked()

    def get_plot_options(self):
        return self.plot_options

    def if_keep(self):
        return self.keep.isChecked()

    def get_tiled_by_type(self):
        index = self.tile_type_selector.currentIndex()
        if index == 0:
            return 'group'
        else:
            return 'run'

    def if_raw(self):
        return self.raw.isChecked()

    def set_raw_checkbox_state(self, state):
        self.raw.setChecked(state)

    def get_selected(self):
        return self.plot_selector.currentText()

    def on_rebin_options_changed(self, slot):
        self.raw.stateChanged.connect(slot)

    def on_plot_type_changed(self, slot):
        self.plot_selector.currentIndexChanged.connect(slot)

    def on_tiled_by_type_changed(self, slot):
        self.tile_type_selector.currentIndexChanged.connect(slot)

    def on_plot_tiled_changed(self, slot):
        self.plot_type_selector.stateChanged.connect(slot)

    def new_plot_figure(self, num_axes):
        # clear old plot
        self.fig.clf()
        if num_axes < 1:
            num_axes = 1
        self.fig, axes = get_plot_fig(overplot=False, ax_properties=None, axes_num=num_axes,
                                      fig=self.fig)
        self.fig.tight_layout()
        self.fig.canvas.draw()

    def addItem(self, plot_type):
        self.plot_selector.addItem(plot_type)

    def get_fig(self):
        return self.fig

    def get_axes(self):
        return self.fig.axes

    def set_fig_titles(self, titles):
        for i, title in enumerate(titles):
            self.fig.axes[i].set_title(title)

    def force_redraw(self):
        self.update_toolbar()
        self.fig.tight_layout()
        self.fig.canvas.draw()

    def update_toolbar(self):
        toolbar = self.fig.canvas.toolbar
        toolbar.update()

    def set_tiled_checkbox_state(self, state):
        self.plot_type_selector.setChecked(state)

    def set_tiled_checkbox_state_quietly(self, state):
        self.plot_type_selector.blockSignals(True)
        self.plot_type_selector.setChecked(state)
        self.plot_type_selector.blockSignals(False)

    def is_tiled_plot(self):
        state = self.plot_type_selector.checkState()
        if state == 0:
            return False
        elif state == 2:
            return True

    def close(self):
        self.fig.canvas.close()
