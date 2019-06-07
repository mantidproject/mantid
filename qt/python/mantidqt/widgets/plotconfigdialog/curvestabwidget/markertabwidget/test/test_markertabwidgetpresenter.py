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
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.presenter import MarkerTabWidgetPresenter


class Test(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fig = figure()
        ax = fig.add_subplot(111)
        plot_args = {'marker': 'v', 'markersize': 1.1, 'markerfacecolor': 'g',
                     'markeredgecolor': 'r'}
        curve = ax.plot([0, 1, 2], [0, 10, 20], **plot_args)[0]
        cls.mock_view = Mock()
        cls.presenter = MarkerTabWidgetPresenter(line=curve, view=cls.mock_view)
        cls.presenter.update_view()

    def test_apply_properties(self):
        line_mock = Mock()
        view_props_mock = Mock()
        with patch.object(self.presenter, 'line', line_mock):
            with patch.object(self.presenter, 'get_view_properties',
                              lambda: view_props_mock):
                self.presenter.apply_properties()
        line_mock.set_marker.assert_called_once_with(view_props_mock.style)
        line_mock.set_markersize.assert_called_once_with(view_props_mock.size)
        line_mock.set_markerfacecolor(view_props_mock.face_color)
        line_mock.set_markeredgecolor(view_props_mock.edge_color)

    def test_set_style_called_on_view_update(self):
        self.mock_view.set_style.assert_called_once_with('triangle_down')

    def test_set_size_called_on_view_update(self):
        self.mock_view.set_size.assert_called_once_with(1.1)

    def test_set_face_color_called_on_view_update(self):
        self.mock_view.set_face_color.assert_called_once_with('#008000')

    def test_set_edge_color_called_on_view_update(self):
        self.mock_view.set_edge_color.assert_called_once_with('#ff0000')


if __name__ == '__main__':
    unittest.main()
