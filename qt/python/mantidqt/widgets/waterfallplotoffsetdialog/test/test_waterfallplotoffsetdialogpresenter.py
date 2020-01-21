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

        self.ax.set_waterfall(True)

        view = Mock()
        view.get_x_offset.return_value = 10
        view.get_y_offset.return_value = 20

        self.presenter = WaterfallPlotOffsetDialogPresenter(fig=self.fig, view=view)

    def test_changing_x_offset_updates_plot(self):
        self.ax.update_waterfall = Mock()
        self.presenter.update_x_offset()

        self.ax.update_waterfall.assert_called_once_with(10, 20)

    def test_changing_y_offset_updates_plot(self):
        self.ax.update_waterfall = Mock()
        self.presenter.update_y_offset()

        self.ax.update_waterfall.assert_called_once_with(10, 20)

    def test_changing_x_offset_also_changes_y_offset_if_kept_in_proportion(self):
        self.presenter.keep_proportion(True)
        # Double the x offset value
        self.presenter.view.get_x_offset.return_value = 20
        self.presenter.update_x_offset()

        # The y offset value should also have doubled
        self.presenter.view.set_y_offset.assert_called_with(40)

    def test_changing_y_offset_also_changes_x_offset_if_kept_in_proportion(self):
        self.presenter.keep_proportion(True)
        # Half the y offset value
        self.presenter.view.get_y_offset.return_value = 10
        self.presenter.update_y_offset()

        # The x offset value should also have halved
        self.presenter.view.set_x_offset.assert_called_with(5)


if __name__ == '__main__':
    unittest.main()
