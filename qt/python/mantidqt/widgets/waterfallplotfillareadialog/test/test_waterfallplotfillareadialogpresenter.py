# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

from matplotlib.collections import PolyCollection
from matplotlib.pyplot import figure

from mantid.plots import datafunctions
from mantid.py3compat.mock import Mock
from mantidqt.widgets.waterfallplotfillareadialog.presenter import WaterfallPlotFillAreaDialogPresenter


class WaterfallPlotFillAreaDialogPresenterTest(unittest.TestCase):

    def setUp(self):
        self.fig = figure()
        self.ax = self.fig.add_subplot(111, projection='mantid')
        self.ax.plot([0, 0], [1, 1])
        self.ax.plot([0, 1], [1, 2])

        self.ax.set_waterfall(True)

        view = Mock()
        self.presenter = WaterfallPlotFillAreaDialogPresenter(fig=self.fig, view=view)

    def test_opening_dialog_calls_init_view(self):
        self.presenter.init_view = Mock()
        self.presenter.init_view.assert_called_once()

    def test_enabling_fill_with_line_colour_creates_fills_that_match_line_colour(self):
        self.presenter.view.enable_fill_group_box.isChecked.return_value = True
        self.presenter.view.use_line_colour_radio_button.isChecked.return_value = True
        self.presenter.set_fill_enabled()

        self.assertTrue(datafunctions.waterfall_fill_is_line_colour(self.ax))

    def test_enabling_fill_with_solid_colour_creates_fills_with_one_colour(self):
        self.presenter.view.enable_fill_group_box.isChecked.return_value = True
        self.presenter.view.use_line_colour_radio_button.isChecked.return_value = False
        self.presenter.view.colour_selector_widget.get_color.return_value = "#ff9900"
        self.presenter.set_fill_enabled()

        fills = [collection for collection in self.ax.collections if isinstance(collection, PolyCollection)]

        self.assertTrue(fill.get_facecolor() == "#ff9900" for fill in fills)

    def test_disabling_fill_removes_fill(self):
        self.ax.set_waterfall_fill(True)
        self.presenter.view.enable_fill_group_box.isChecked.return_value = False
        self.presenter.set_fill_enabled()

        self.assertFalse(self.ax.waterfall_has_fill())


if __name__ == '__main__':
    unittest.main()
