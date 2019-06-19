# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')
from matplotlib.pyplot import figure

from mantid.py3compat.mock import Mock, patch
from mantidqt.widgets.plotconfigdialog.presenter import PlotConfigDialogPresenter


PRESENTER_REF = 'mantidqt.widgets.plotconfigdialog.presenter.'


class PlotConfigDialogPresenterTest(unittest.TestCase):

    def setUp(self):
        self.axes_patch = patch(PRESENTER_REF + 'AxesTabWidgetPresenter',
                                new=Mock())
        self.axes_mock = self.axes_patch.start()
        self.curves_patch = patch(PRESENTER_REF + 'CurvesTabWidgetPresenter',
                                  new=Mock())
        self.curves_mock = self.curves_patch.start()
        self.images_patch = patch(PRESENTER_REF + 'ImagesTabWidgetPresenter',
                                  new=Mock())
        self.images_mock = self.images_patch.start()

    def tearDown(self):
        self.axes_patch.stop()
        self.curves_patch.stop()
        self.images_patch.stop()

    def assert_called_x_times_with(self, x, call_args, mock):
        self.assertEqual(x, mock.call_count)
        self.assertEqual(call_args, [arg[0][0] for arg in mock.call_args_list])

    def test_correct_tabs_present_axes_only(self):
        fig = figure()
        fig.add_subplot(111)
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.axes_mock.return_value, None, None]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        mock_view.add_tab_widget.assert_called_once_with(
            (self.axes_mock.return_value.view, 'Axes'))

    def test_correct_tabs_present_axes_and_curve_no_errors(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0], [0])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.axes_mock.return_value,
                                   self.curves_mock.return_value, None]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.curves_mock.return_value.view, 'Curves')]
        self.assert_called_x_times_with(2, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_correct_tabs_present_axes_and_curve_with_errors(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.errorbar([0], [0], yerr=[1])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.axes_mock.return_value,
                                   self.curves_mock.return_value, None]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.curves_mock.return_value.view, 'Curves')]
        self.assert_called_x_times_with(2, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_correct_tabs_present_axes_and_image_colormesh(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.pcolormesh([[0, 1], [1, 0]])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.axes_mock.return_value,
                                   None, self.images_mock.return_value]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.images_mock.return_value.view, 'Images')]
        self.assert_called_x_times_with(2, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_correct_tabs_present_axes_and_image_imshow(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.imshow([[0, 1], [1, 0]])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.axes_mock.return_value,
                                   None, self.images_mock.return_value]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.images_mock.return_value.view, 'Images')]
        self.assert_called_x_times_with(2, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_correct_tabs_present_axes_curves_and_image(self):
        fig = figure()
        ax = fig.add_subplot(211)
        ax.imshow([[0, 1], [1, 0]])
        ax1 = fig.add_subplot(212)
        ax1.errorbar([0], [0], yerr=[1])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.axes_mock.return_value,
                                   self.curves_mock.return_value,
                                   self.images_mock.return_value]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.curves_mock.return_value.view, 'Curves'),
                              (self.images_mock.return_value.view, 'Images')]
        self.assert_called_x_times_with(3, expected_call_args,
                                        mock_view.add_tab_widget)


if __name__ == '__main__':
    unittest.main()
