# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from unittest.mock import Mock, patch
from mantidqt.widgets.waterfallplotfillareadialog.presenter import WaterfallPlotFillAreaDialogPresenter


class WaterfallPlotFillAreaDialogPresenterTest(unittest.TestCase):
    def setUp(self):
        self.fig = Mock()
        self.ax = Mock()

        self.fig.get_axes.return_value = [self.ax]

        view = Mock()
        with patch("mantidqt.widgets.waterfallplotfillareadialog.presenter.WaterfallPlotFillAreaDialogPresenter.init_view"):
            self.presenter = WaterfallPlotFillAreaDialogPresenter(fig=self.fig, view=view)

    @patch("mantidqt.widgets.waterfallplotfillareadialog.presenter.WaterfallPlotFillAreaDialogPresenter.init_view")
    def test_opening_dialog_calls_init_view(self, patched_init):
        self.presenter = WaterfallPlotFillAreaDialogPresenter(fig=self.fig, view=Mock())
        patched_init.assert_called_once()

    @patch("mantidqt.widgets.waterfallplotfillareadialog.presenter.datafunctions")
    def test_enabling_fill_with_line_colour_creates_fills_that_match_line_colour(self, patched_data_func):
        self.presenter.view.enable_fill_group_box.isChecked.return_value = True
        self.presenter.view.use_line_colour_radio_button.isChecked.return_value = True
        self.presenter.set_fill_enabled()

        patched_data_func.line_colour_fill.assert_called_once_with(self.ax)

    @patch("mantidqt.widgets.waterfallplotfillareadialog.presenter.datafunctions")
    def test_enabling_fill_with_solid_colour_creates_fills_with_one_colour(self, patched_data_func):
        self.presenter.view.enable_fill_group_box.isChecked.return_value = True
        self.presenter.view.use_line_colour_radio_button.isChecked.return_value = False
        color = "#ff9900"
        self.presenter.view.colour_selector_widget.get_color.return_value = color
        self.presenter.set_fill_enabled()

        patched_data_func.solid_colour_fill.assert_called_once_with(self.ax, color)

    def test_disabling_fill_removes_fill(self):
        self.ax.set_waterfall_fill(True)
        self.ax.set_waterfall_fill.assert_called_once_with(True)
        self.presenter.view.enable_fill_group_box.isChecked.return_value = False

        self.ax.set_waterfall_fill.reset_mock()
        self.presenter.set_fill_enabled()
        self.ax.set_waterfall_fill.assert_called_once_with(False)


if __name__ == "__main__":
    unittest.main()
