# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.collections import PolyCollection

from mantidqt.widgets.plotconfigdialog.colorselector import convert_color_to_hex
from mantidqt.widgets.waterfallplotfillareadialog.view import WaterfallPlotFillAreaDialogView


class WaterfallPlotFillAreaDialogPresenter:

    def __init__(self, fig, view=None, parent=None):
        self.fig = fig
        self.ax = fig.get_axes()[0]
        if view:
            self.view = view
        else:
            self.view = WaterfallPlotFillAreaDialogView(parent)

        self.init_view()
        self.view.show()

        # Signals
        self.view.close_push_button.clicked.connect(self.view.close)
        self.view.enable_fill_group_box.clicked.connect(lambda: self.set_fill_enabled())
        self.view.use_line_colour_radio_button.clicked.connect(self.line_colour_fill)
        self.view.use_solid_colour_radio_button.clicked.connect(self.solid_colour_fill)
        self.view.colour_selector_widget.line_edit.textChanged.connect(self.solid_colour_fill)

    def init_view(self):
        # This function sets the correct values in the menu when it is first opened.

        if self.ax.waterfall_has_fill():
            self.view.enable_fill_group_box.setChecked(True)

            if self.ax.waterfall_fill_is_line_colour():
                self.view.use_line_colour_radio_button.setChecked(True)
            else:
                self.view.use_solid_colour_radio_button.setChecked(True)
                poly = next(poly_collection for poly_collection in self.ax.collections
                            if isinstance(poly_collection, PolyCollection))
                self.view.colour_selector_widget.set_color(convert_color_to_hex(poly.get_facecolor().tolist()[0]))

    def set_fill_enabled(self):
        if self.view.enable_fill_group_box.isChecked():
            if self.view.use_line_colour_radio_button.isChecked():
                self.line_colour_fill()
            else:
                self.solid_colour_fill()
        else:
            self.remove_fill()

    def line_colour_fill(self):
        # Add the fill areas if there aren't any already.
        if not any(isinstance(collection, PolyCollection) for collection in self.ax.collections):
            self.create_fill()

        i = 0
        for collection in self.ax.collections:
            if isinstance(collection, PolyCollection):
                colour = self.ax.get_lines()[i].get_color()
                collection.set_color(colour)
                # Only want the counter to iterate if the current collection is a PolyCollection (the fill areas) since
                # the axes may have other collections which can be ignored.
                i = i + 1

        self.fig.canvas.draw()

    def solid_colour_fill(self):
        # If the colour selector has been changed then presumably the user wants to set a custom fill colour
        # so that option is checked if it wasn't already.
        if not self.view.use_solid_colour_radio_button.isChecked():
            self.view.use_solid_colour_radio_button.setChecked(True)

        # Add the fill areas if there aren't any already.
        if not any(isinstance(collection, PolyCollection) for collection in self.ax.collections):
            self.create_fill()

        colour = self.view.colour_selector_widget.get_color()

        for i, collection in enumerate(self.ax.collections):
            if isinstance(collection, PolyCollection):
                # This function is called every time the colour line edit is changed so it's possible
                # that the current input is not a valid colour, such as if the user hasn't finished entering
                # a colour. So if setting the colour fails, the function just stops.
                try:
                    collection.set_color(colour)
                except:
                    return

        self.fig.canvas.draw()

    def create_fill(self):
        self.ax.waterfall_create_fill()

    def remove_fill(self):
        self.ax.waterfall_remove_fill()
