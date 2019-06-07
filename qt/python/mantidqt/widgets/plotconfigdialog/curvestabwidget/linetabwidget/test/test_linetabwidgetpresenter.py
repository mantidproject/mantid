# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure

from mantid.py3compat.mock import Mock, patch
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget.presenter import LineTabWidgetPresenter


class LineTabWidgetPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fig = figure()
        ax = fig.add_subplot(111)
        plot_args = {'linewidth': 1.1, 'drawstyle': 'steps', 'color': 'g',
                     'linestyle': '--'}
        curve = ax.plot([0, 1, 2], [0, 10, 20], **plot_args)[0]
        cls.mock_view = Mock()
        cls.presenter = LineTabWidgetPresenter(line=curve, view=cls.mock_view)
        cls.presenter.update_view()

    def test_set_style_called_on_view_update(self):
        self.mock_view.set_style.assert_called_once_with('dashed')

    def test_set_draw_style_called_on_view_update(self):
        self.mock_view.set_draw_style.assert_called_once_with('steps')

    def test_set_width_called_on_view_update(self):
        self.mock_view.set_width.assert_called_once_with(1.1)

    def test_set_color_called_on_view_update(self):
        self.mock_view.set_color.assert_called_once_with('#008000')

    def test_apply_properties(self):
        line_mock = Mock()
        view_props_mock = Mock()
        with patch.object(self.presenter, 'line', line_mock):
            with patch.object(self.presenter, 'get_view_properties',
                              lambda: view_props_mock):
                self.presenter.apply_properties()
        line_mock.set_linestyle.assert_called_once_with(view_props_mock.style)
        line_mock.set_drawstyle.assert_called_once_with(view_props_mock.draw_style)
        line_mock.set_linewidth.assert_called_once_with(view_props_mock.width)
        line_mock.set_color.assert_called_once_with(view_props_mock.color)


if __name__ == '__main__':
    unittest.main()
