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

import sys
from mantidqt.widgets.plotselector.model import PlotSelectorModel
from mantidqt.widgets.plotselector.presenter import PlotSelectorPresenter
from mantidqt.widgets.plotselector.widget import PlotSelectorWidget

import unittest
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class PlotSelectorPresenterTest(unittest.TestCase):

    def setUp(self):
        self.widget = mock.Mock(spec=PlotSelectorWidget)
        self.widget.get_filter_text = mock.Mock(return_value="")

        self.model = mock.Mock(spec=PlotSelectorModel)
        self.model.configure_mock(plot_list=["Plot1", "Plot2", "Plot3"])

        self.presenter = PlotSelectorPresenter(None, self.widget, self.model)
        self.presenter.widget = self.widget
        self.presenter.model = self.model

    def convert_list_to_calls(self, list_to_convert):
        call_list = []
        for item in list_to_convert:
            call_list.append(mock.call(item))
        return call_list

    # ------------------------ Closing Tests ------------------------

    def test_close_button_pressed_single_plot(self):
        self.widget.get_all_selected_plot_names = mock.Mock(return_value=["Plot1"])

        self.presenter.close_button_clicked()
        self.model.close_plot.assert_called_once_with("Plot1")

    def test_close_button_pressed_multiple_plots(self):
        self.widget.get_all_selected_plot_names = mock.Mock(return_value=["Plot1", "Plot3"])

        self.presenter.close_button_clicked()
        self.assertEqual(self.model.close_plot.call_count, 2)
        self.model.close_plot.assert_has_calls(self.convert_list_to_calls(["Plot1", "Plot3"]),
                                               any_order=True)

    def test_close_button_pressed_with_no_plots_open(self):
        self.widget.get_all_selected_plot_names = mock.Mock(return_value=[])
        self.model.configure_mock(plot_list=[])

        self.presenter.close_button_clicked()
        self.assertEqual(self.model.close_plot.call_count, 0)

    def test_close_button_pressed_with_no_plots_selected(self):
        self.widget.get_all_selected_plot_names = mock.Mock(return_value=[])

        self.presenter.close_button_clicked()
        self.assertEqual(self.model.close_plot.call_count, 0)


if __name__ == '__main__':
    unittest.main()
