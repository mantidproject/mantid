# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from unittest.mock import Mock, call, patch

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure, subplots

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
        self.legend_patch = patch(PRESENTER_REF + 'LegendTabWidgetPresenter',
                                  new=Mock())
        self.legend_mock = self.legend_patch.start()

    def tearDown(self):
        self.axes_patch.stop()
        self.curves_patch.stop()
        self.images_patch.stop()
        self.legend_patch.stop()

    def assert_called_x_times_with(self, x, call_args, mock):
        self.assertEqual(x, mock.call_count)
        self.assertEqual(call_args, [arg[0][0] for arg in mock.call_args_list])

    def test_correct_tabs_present_axes_only(self):
        fig = figure()
        fig.add_subplot(111)
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [None, self.axes_mock.return_value, None, None]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        mock_view.add_tab_widget.assert_called_once_with(
            (self.axes_mock.return_value.view, 'Axes'))

    def test_correct_tabs_present_axes_and_curve_no_errors(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0], [0])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [None, self.axes_mock.return_value,
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
        expected_presenter_list = [None, self.axes_mock.return_value,
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
        expected_presenter_list = [None, self.axes_mock.return_value,
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
        expected_presenter_list = [None, self.axes_mock.return_value,
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
        expected_presenter_list = [None, self.axes_mock.return_value,
                                   self.curves_mock.return_value,
                                   self.images_mock.return_value]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.curves_mock.return_value.view, 'Curves'),
                              (self.images_mock.return_value.view, 'Images')]
        self.assert_called_x_times_with(3, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_correct_tabs_present_axes_curves_and_legend(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0])
        ax.legend(['Label'])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [self.legend_mock.return_value,
                                   self.axes_mock.return_value,
                                   self.curves_mock.return_value, None]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.curves_mock.return_value.view, 'Curves'),
                              (self.legend_mock.return_value.view, 'Legend')]
        self.assert_called_x_times_with(3, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_correct_tabs_present_axes_and_curve_legend_has_no_text(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0], [0])
        ax.legend()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        expected_presenter_list = [None, self.axes_mock.return_value,
                                   self.curves_mock.return_value, None]
        self.assertEqual(expected_presenter_list, presenter.tab_widget_presenters)
        expected_call_args = [(self.axes_mock.return_value.view, 'Axes'),
                              (self.curves_mock.return_value.view, 'Curves')]
        self.assert_called_x_times_with(2, expected_call_args,
                                        mock_view.add_tab_widget)

    def test_tabs_present_updated_properties_from_figure_when_apply_clicked(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0], [0])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)

        # use mock manager to ensure all user properties are applied before view update
        mock_axes_presenter = presenter.tab_widget_presenters[1]
        mock_curves_presenter = presenter.tab_widget_presenters[2]
        mock_manager = Mock()
        mock_manager.attach_mock(mock_axes_presenter, "mock_axes_presenter")
        mock_manager.attach_mock(mock_curves_presenter, "mock_curves_presenter")

        presenter.apply_properties()
        mock_manager.assert_has_calls([
            call.mock_curves_presenter.apply_properties,
            call.mock_axes_presenter.apply_properties,
            call.mock_curves_presenter.update_view,
            call.mock_axes_presenter.update_view
        ])

    def test_forget_tab_from_presenter_sets_presenter_and_view_to_none(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0], [0])
        ax.legend()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)

        mock_curves_presenter = presenter.tab_widget_presenters[2]
        mock_curves_view = mock_curves_presenter.view

        self.assertTrue(mock_curves_presenter in presenter.tab_widget_presenters)
        self.assertTrue((mock_curves_view, 'Curves') in presenter.tab_widget_views)

        presenter.forget_tab_from_presenter(mock_curves_presenter)

        self.assertTrue(mock_curves_presenter not in presenter.tab_widget_presenters)
        self.assertTrue((mock_curves_view, 'Curves') not in presenter.tab_widget_views)

    def test_configure_curves_tab_fails_silently_when_curves_tab_not_exists(self):
        fig = figure()
        ax = fig.add_subplot(111)
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        self.assertIsNone(presenter.tab_widget_presenters[2])

        presenter.configure_curves_tab(ax, None)

        mock_view.set_current_tab_widget.assert_not_called()

    def test_configure_curves_tab_fails_silently_when_no_curves_on_axes(self):
        fig, (ax0, ax1) = subplots(2, subplot_kw={'projection': 'mantid'})
        ax0.plot([0], [0])  # One axes must have a curve for curves tab to exist
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        mock_curves_presenter = presenter.tab_widget_presenters[2]
        mock_curves_presenter.set_axes_from_object.side_effect = ValueError("Axes object does not exist in curves tab")

        presenter.configure_curves_tab(ax1, None)

        mock_curves_presenter.set_axes_from_object.assert_called()
        mock_view.set_current_tab_widget.assert_not_called()

    def test_configure_curves_tab_fails_silently_when_curve_not_found_in_curves_tab(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.plot([0], [0])  # Must plot curve for curves tab to exist, hence why we dont use this in the call
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        mock_curves_presenter = presenter.tab_widget_presenters[2]
        mock_curves_presenter.set_curve_from_object.side_effect = ValueError("Curve object does not exist in curves tab")
        mock_curves_view, _ = presenter.tab_widget_views[1]

        presenter.configure_curves_tab(ax, Mock())

        mock_curves_presenter.set_axes_from_object.assert_called()
        mock_view.set_current_tab_widget.assert_called_with(mock_curves_view)
        mock_view.set_current_tab_widget.assert_called()

    def test_configure_curves_tab_succeeds_when_curve_and_axes_exist(self):
        fig = figure()
        ax = fig.add_subplot(111)
        curve = ax.plot([0], [0])
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        mock_curves_presenter = presenter.tab_widget_presenters[2]
        mock_curves_view, _ = presenter.tab_widget_views[1]

        presenter.configure_curves_tab(ax, curve)

        mock_curves_presenter.set_axes_from_object.assert_called()
        mock_view.set_current_tab_widget.assert_called_with(mock_curves_view)
        mock_view.set_current_tab_widget.assert_called()

    def test_apply_properties_calls_error_callback_when_exception_raised_in_canvas_draw(self):
        canvas_draw_exception = Exception("Exception in canvas.draw")

        def raise_():
            raise canvas_draw_exception

        fig = figure()
        fig.canvas.draw = Mock(side_effect=lambda: raise_())
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        presenter.success_callback = Mock()
        presenter.error_callback = Mock()
        mock_presenters = [Mock(), Mock(), Mock(), Mock()]
        presenter.tab_widget_presenters = mock_presenters

        presenter.apply_properties()

        presenter.error_callback.assert_called_with(str(canvas_draw_exception))
        presenter.success_callback.assert_not_called()
        for mock in presenter.tab_widget_presenters:
            mock.update_view.assert_not_called()

    def test_apply_properties_calls_success_callback_on_canvas_draw_success(self):
        fig = figure()
        fig.canvas.draw = Mock()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        presenter.success_callback = Mock()
        presenter.error_callback = Mock()

        presenter.apply_properties()

        presenter.success_callback.assert_called()
        presenter.error_callback.assert_not_called()

    def test_apply_all_properties_and_exist_doesnt_exit_if_error_state_true(self):
        fig = figure()
        fig.canvas.draw = Mock()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        presenter.apply_properties = Mock()
        presenter.error_state = True

        presenter.apply_properties_and_exit()

        mock_view.close.assert_not_called()

    def test_apply_all_properties_and_exist_does_exit_if_error_state_false(self):
        fig = figure()
        fig.canvas.draw = Mock()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)
        presenter.apply_properties = Mock()
        presenter.error_state = False

        presenter.apply_properties_and_exit()

        mock_view.close.assert_called()

    def test_error_callback(self):
        exception_string = "test string"

        fig = figure()
        fig.canvas.draw = Mock()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)

        presenter.error_callback(exception_string)

        mock_view.set_error_text.assert_called_with(exception_string)
        self.assertTrue(presenter.error_state)

    def test_success_callback(self):
        fig = figure()
        fig.canvas.draw = Mock()
        mock_view = Mock()
        presenter = PlotConfigDialogPresenter(fig, mock_view)

        presenter.success_callback()

        mock_view.set_error_text.assert_called_with(None)
        self.assertFalse(presenter.error_state)


if __name__ == '__main__':
    unittest.main()
