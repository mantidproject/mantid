# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

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
        new_x = self.view.get_x_offset()

        if self.keep_proportion_on:
            if new_x == 0:
                self.view.keep_proportion_check_box.setChecked(False)
                self.keep_proportion_on = False
            else:
                self.view.set_y_offset(int(self.view.get_x_offset() / self.proportion))

        self.ax.update_waterfall(new_x, self.view.get_y_offset())

    def update_y_offset(self):
        new_y = self.view.get_y_offset()

        if self.view.y_offset_spin_box.hasFocus() and self.keep_proportion_on:
            if new_y == 0:
                self.view.keep_proportion_check_box.setChecked(False)
                self.keep_proportion_on = False
            else:
                self.view.set_x_offset(int(self.view.get_y_offset() * self.proportion))

        self.ax.update_waterfall(self.view.get_x_offset(), new_y)

    def keep_proportion(self, enabled):
        if enabled:
            x, y = float(self.view.get_x_offset()), float(self.view.get_y_offset())
            if x == 0 or y == 0:
                self.view.keep_proportion_check_box.setChecked(False)
                return

            self.proportion = x / y
            self.keep_proportion_on = True
        else:
            self.keep_proportion_on = False
