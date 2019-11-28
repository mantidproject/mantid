# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.waterfallplotoffsetdialog.view import WaterfallPlotOffsetDialogView


class WaterfallPlotOffsetDialogPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        if view:
            self.view = view
        else:
            self.view = WaterfallPlotOffsetDialogView(parent)

        self.view.set_x_offset(fig.get_axes()[0].waterfall_x_offset)
        self.view.set_y_offset(fig.get_axes()[0].waterfall_y_offset)

        self.view.show()

        self.view.close_push_button.clicked.connect(self.view.close)

        self.view.y_offset_spin_box.valueChanged.connect(lambda: self.update_y_offset())
        self.view.x_offset_spin_box.valueChanged.connect(lambda: self.update_x_offset())

    def update_x_offset(self):
        ax = self.fig.get_axes()[0]
        ax.set_waterfall_x_offset(self.view.get_x_offset())

    def update_y_offset(self):
        ax = self.fig.get_axes()[0]
        ax.set_waterfall_y_offset(self.view.get_y_offset())