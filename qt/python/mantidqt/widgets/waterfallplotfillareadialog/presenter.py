# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.collections import PolyCollection

from mantidqt.widgets.waterfallplotfillareadialog.view import WaterfallPlotFillAreaDialogView


class WaterfallPlotFillAreaDialogPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        self.ax = fig.get_axes()[0]
        if view:
            self.view = view
        else:
            self.view = WaterfallPlotFillAreaDialogView(parent)

        self.view.show()
        self.view.close_push_button.clicked.connect(self.view.close)
        self.view.enable_fill_group_box.clicked.connect(self.set_fill_enabled)
        self.view.use_line_colour_radio_button.clicked.connect(self.line_colour_fill)

    def set_fill_enabled(self, enabled):
        if enabled:
            if self.view.use_line_colour_radio_button.isChecked():
                self.line_colour_fill()

    def line_colour_fill(self):
        if len(self.ax.collections) == 0:
            self.create_fill()

        j = 0
        for i, collection in enumerate(self.ax.collections):
            if isinstance(collection, PolyCollection):
                colour = self.ax.get_lines()[j].get_color()
                collection.set_color(colour)
                collection.set_zorder(1-j)
                j = j + 1

        self.fig.canvas.draw()

    def create_fill(self):
        for line in self.ax.get_lines():
            self.ax.fill_between(line.get_xdata(), line.get_ydata())