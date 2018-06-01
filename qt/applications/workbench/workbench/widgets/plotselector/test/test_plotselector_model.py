#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import, division, print_function

from workbench.widgets.plotselector.model import PlotSelectorModel
from workbench.widgets.plotselector.presenter import PlotSelectorPresenter

import unittest
try:
    from unittest import mock
except ImportError:
    import mock


class PlotSelectorModelTest(unittest.TestCase):

    def side_effects_manager(self, plot_name):
        if plot_name == "Plot1":
            return self.figure_manager
        return None

    def side_effects_number(self, plot_name):
        if plot_name == "Plot1":
            return 42
        return None

    def setUp(self):
        self.presenter = mock.Mock(spec=PlotSelectorPresenter)

        self.figure_manager = mock.Mock()
        self.figure_manager.show = mock.Mock()

        self.global_figure_manager = mock.Mock()
        self.global_figure_manager.add_observer = mock.Mock()
        self.global_figure_manager.get_figure_manager_from_name = mock.Mock(side_effect=self.side_effects_manager)
        self.global_figure_manager.get_figure_number_from_name = mock.Mock(side_effect=self.side_effects_number)
        self.global_figure_manager.destroy = mock.Mock()

        self.model = PlotSelectorModel(self.presenter, self.global_figure_manager)
        self.model.plot_list = ["Plot1", "Plot2"]

    def test_observer_added_during_setup(self):
        self.assertEqual(self.global_figure_manager.add_observer.call_count, 1)

    def test_notify_calls_update_in_presenter(self):
        self.model.notify()
        self.assertEqual(self.presenter.update_plot_list.call_count, 1)
        self.model.notify()
        self.assertEqual(self.presenter.update_plot_list.call_count, 2)

    def test_make_plot_active_calls_current_figure(self):
        self.model.make_plot_active("Plot1")
        self.assertEqual(self.figure_manager.show.call_count, 1)

    def test_make_plot_active_for_invalid_name_does_nothing(self):
        self.model.make_plot_active("NotAPlot")
        self.figure_manager.show.assert_not_called()

    def test_close_plot_calls_destroy_in_current_figure(self):
        self.model.close_plot("Plot1")
        self.global_figure_manager.destroy.assert_called_once_with(42)

    def test_close_plot_for_invalid_name_does_noting(self):
        self.model.close_plot("NotAPlot")
        self.global_figure_manager.destroy.assert_not_called()


if __name__ == '__main__':
    unittest.main()
