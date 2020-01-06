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
        self.ax = fig.get_axes()[0]
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
        self.view.keep_proportion_check_box.clicked.connect(self.keep_proportion)

        self.keep_proportion_on = False

    def update_x_offset(self):
        prev_x = self.ax.waterfall_x_offset
        new_x = self.view.get_x_offset()
        self.ax.set_waterfall_x_offset(new_x)

        if self.view.x_offset_spin_box.hasFocus() and self.keep_proportion_on:
            if prev_x == 0 or new_x == 0:
                self.view.keep_proportion_check_box.setChecked(False)
                self.keep_proportion_on = False
                return

            self.view.set_y_offset(self.view.get_x_offset() / self.proportion)

    def update_y_offset(self):
        prev_y = self.ax.waterfall_y_offset
        new_y = self.view.get_y_offset()
        self.ax.set_waterfall_y_offset(new_y)

        if self.view.y_offset_spin_box.hasFocus() and self.keep_proportion_on:
            if prev_y == 0 or new_y == 0:
                self.view.keep_proportion_check_box.setChecked(False)
                self.keep_proportion_on = False
                return

            self.view.set_x_offset(self.view.get_y_offset() * self.proportion)

    def keep_proportion(self, enabled):
        if enabled:
            self.keep_proportion_on = True
            self.proportion = float(self.view.get_x_offset()) / float(self.view.get_y_offset())
        else:
            self.keep_proportion_on = False
