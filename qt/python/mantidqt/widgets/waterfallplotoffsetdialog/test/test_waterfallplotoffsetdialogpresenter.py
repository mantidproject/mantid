# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

from matplotlib.pyplot import figure

from mantid.plots import MantidAxes # noqa
from mantid.py3compat.mock import Mock
from mantidqt.widgets.waterfallplotoffsetdialog.presenter import WaterfallPlotOffsetDialogPresenter


class WaterfallPlotOffsetDialogPresenterTest(unittest.TestCase):

    def setUp(self):
        self.fig = figure()
        self.ax = self.fig.add_subplot(111, projection='mantid')
        self.ax.plot([0, 0], [1, 1])
        self.ax.plot([0, 1], [1, 2])

        self.ax.set_waterfall_toolbar_options_enabled = Mock()
        self.ax.convert_to_waterfall()

        self.presenter = WaterfallPlotOffsetDialogPresenter(fig=self.fig, view=Mock())

    def test_changing_x_offset_updates_plot(self):
        self.ax.set_waterfall_x_offset = Mock()
        self.presenter.update_x_offset()

        self.assertTrue(self.ax.set_waterfall_x_offset.called)

    def test_changing_y_offset_updates_plot(self):
        self.ax.set_waterfall_y_offset = Mock()
        self.presenter.update_y_offset()

        self.assertTrue(self.ax.set_waterfall_y_offset.called)

    def test_changing_x_offset_also_changes_y_offset_if_kept_in_proportion(self):
        self.presenter.keep_proportion_on = True
        self.presenter.view.get_x_offset.return_value = 10
        self.presenter.proportion = 2
        self.presenter.update_x_offset()

        self.presenter.view.set_y_offset.assert_called_once_with(5)

    def test_changing_y_offset_also_changes_x_offset_if_kept_in_proportion(self):
        self.presenter.keep_proportion_on = True
        self.presenter.view.get_y_offset.return_value = 10
        self.presenter.proportion = 2
        self.presenter.update_y_offset()

        self.presenter.view.set_y_offset.assert_called_once_with(20)


if __name__ == '__main__':
    unittest.main()
