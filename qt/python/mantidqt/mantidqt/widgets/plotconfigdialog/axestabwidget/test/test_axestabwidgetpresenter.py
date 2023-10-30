# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from matplotlib import use as mpl_use

mpl_use("Agg")
from matplotlib.pyplot import figure

from unittest import mock
from mantidqt.widgets.plotconfigdialog import generate_ax_name, get_axes_names_dict
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.presenter import AxesTabWidgetPresenter as Presenter


class AxesTabWidgetPresenterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fig = figure()
        ax = cls.fig.add_subplot(211)
        ax.plot([0, 1], [10, 12], "rx")
        cls.title = "My Axes"
        ax.set_title(cls.title)
        cls.x_label = "X"
        ax.set_xlabel(cls.x_label)
        cls.x_scale = "linear"
        ax.set_xscale(cls.x_scale)
        cls.y_label = "Y"
        ax.set_ylabel(cls.y_label)
        cls.y_scale = "log"
        ax.set_yscale(cls.y_scale)
        cls.canvas_color = (1.0, 1.0, 0.0, 1.0)
        ax.set_facecolor(cls.canvas_color)
        ax2 = cls.fig.add_subplot(212)
        ax2.plot([-1000000, 10000], [10, 12], "rx")

    def _generate_presenter(self):
        mock_view = mock.Mock(get_selected_ax_name=lambda: "My Axes: (0, 0)", get_axis=lambda: "x", get_properties=lambda: {})
        return Presenter(self.fig, view=mock_view)

    def test_generate_ax_name_returns_correct_name(self):
        ax0_name = generate_ax_name(self.fig.get_axes()[0])
        self.assertEqual("My Axes: (0, 0)", ax0_name)
        ax1_name = generate_ax_name(self.fig.get_axes()[1])
        self.assertEqual("(1, 0)", ax1_name)

    def test_apply_properties_calls_setters_with_correct_properties(self):
        ax_mock = mock.MagicMock()
        ax_mock.get_autoscalex_on = mock.Mock(return_value=False)
        ax_mock.get_autoscaley_on = mock.Mock(return_value=False)
        presenter = self._generate_presenter()
        with mock.patch.object(presenter, "get_selected_ax", lambda: ax_mock):
            with mock.patch.object(presenter, "update_view", lambda: None):
                with mock.patch.object(presenter, "axis_changed", lambda: None):
                    presenter.current_view_props = presenter.get_selected_ax_properties()
                    presenter.apply_properties()
                    # Mock properties object and view then test that the view's setters
                    # are called with the correct property values
                    ax_mock.set_title.assert_called_once_with(presenter.current_view_props.title)
                    ax_mock.set_xlim.assert_called_once_with(presenter.current_view_props.xlim)
                    ax_mock.set_xlabel.assert_called_once_with(presenter.current_view_props.xlabel)
                    ax_mock.set_xscale.assert_called_once_with(presenter.current_view_props.xscale.lower())
                    ax_mock.set_ylim.assert_called_once_with(presenter.current_view_props.ylim)
                    ax_mock.set_ylabel.assert_called_once_with(presenter.current_view_props.ylabel)
                    ax_mock.set_yscale.assert_called_once_with(presenter.current_view_props.yscale.lower())
                    ax_mock.minorticks_on.assert_called_once()
                    ax_mock.set_facecolor.assert_called_once_with(presenter.current_view_props.canvas_color)

    @mock.patch("mantid.plots.axesfunctions.update_colorplot_datalimits")
    def test_apply_properties_calls_setters_with_correct_properties_autoscale(self, mock_colorplot_autoscale):
        ax_mock = mock.MagicMock(images=None)
        ax_mock.get_autoscalex_on = mock.Mock(return_value=True)
        ax_mock.get_autoscaley_on = mock.Mock(return_value=True)
        presenter = self._generate_presenter()
        with mock.patch.object(presenter, "get_selected_ax", lambda: ax_mock):
            with mock.patch.object(presenter, "update_view", lambda: None):
                with mock.patch.object(presenter, "axis_changed", lambda: None):
                    presenter.current_view_props = presenter.get_selected_ax_properties()
                    presenter.apply_properties()
                    # Mock properties object and view then test that the view's setters
                    # are called with the correct property values
                    ax_mock.set_title.assert_called_once_with(presenter.current_view_props.title)
                    ax_mock.autoscale.assert_has_calls([mock.call(True, axis="x")])
                    ax_mock.set_xlabel.assert_called_once_with(presenter.current_view_props.xlabel)
                    ax_mock.set_xscale.assert_called_once_with(presenter.current_view_props.xscale.lower())
                    ax_mock.autoscale.assert_has_calls([mock.call(True, axis="y")])
                    ax_mock.set_ylabel.assert_called_once_with(presenter.current_view_props.ylabel)
                    ax_mock.set_yscale.assert_called_once_with(presenter.current_view_props.yscale.lower())
                    ax_mock.minorticks_on.assert_called_once()
                    ax_mock.set_facecolor.assert_called_once_with(presenter.current_view_props.canvas_color)
                    ax_mock.set_xlim.assert_not_called()
                    ax_mock.set_ylim.assert_not_called()
                    # update_colorplot_datalimits shouldn't be called for this axes
                    mock_colorplot_autoscale.assert_not_called()

    def test_apply_all_properties_calls_setters_with_correct_properties(self):
        ax_mock_1 = mock.MagicMock()
        ax_mock_2 = mock.MagicMock()
        ax_mock_1.get_autoscalex_on = mock.Mock(return_value=False)
        ax_mock_1.get_autoscaley_on = mock.Mock(return_value=False)
        presenter = self._generate_presenter()
        presenter.axes_names_dict = {"1": ax_mock_1, "2": ax_mock_2}
        with mock.patch.object(presenter, "get_selected_ax", lambda: ax_mock_1):
            with mock.patch.object(presenter, "update_view", lambda: None):
                with mock.patch.object(presenter, "axis_changed", lambda: None):
                    presenter.current_view_props = presenter.get_selected_ax_properties()
                    presenter.apply_all_properties()
                    # Mock properties object and view then test that the view's setters
                    # are called with the correct property values
                    for key in presenter.axes_names_dict.keys():
                        ax_mock = presenter.axes_names_dict[key]
                        ax_mock.set_xlim.assert_called_once_with(presenter.current_view_props.xlim)
                        ax_mock.set_xlabel.assert_called_once_with(presenter.current_view_props.xlabel)
                        ax_mock.set_xscale.assert_called_once_with(presenter.current_view_props.xscale.lower())
                        ax_mock.set_ylim.assert_called_once_with(presenter.current_view_props.ylim)
                        ax_mock.set_ylabel.assert_called_once_with(presenter.current_view_props.ylabel)
                        ax_mock.set_yscale.assert_called_once_with(presenter.current_view_props.yscale.lower())
                        ax_mock.set_facecolor.assert_called_once_with(presenter.current_view_props.canvas_color)

    def test_apply_all_properties_calls_setters_with_correct_properties_autoscale(self):
        ax_mock_1 = mock.MagicMock(images=None)
        ax_mock_2 = mock.MagicMock(images=None)
        ax_mock_1.get_autoscalex_on = mock.Mock(return_value=True)
        ax_mock_1.get_autoscaley_on = mock.Mock(return_value=True)
        presenter = self._generate_presenter()
        presenter.axes_names_dict = {"1": ax_mock_1, "2": ax_mock_2}
        with mock.patch.object(presenter, "get_selected_ax", lambda: ax_mock_1):
            with mock.patch.object(presenter, "update_view", lambda: None):
                with mock.patch.object(presenter, "axis_changed", lambda: None):
                    presenter.current_view_props = presenter.get_selected_ax_properties()
                    presenter.apply_all_properties()
                    # Mock properties object and view then test that the view's setters
                    # are called with the correct property values
                    for key in presenter.axes_names_dict.keys():
                        ax_mock = presenter.axes_names_dict[key]
                        ax_mock.autoscale.assert_has_calls([mock.call(True, axis="x")])
                        ax_mock.set_xlabel.assert_called_once_with(presenter.current_view_props.xlabel)
                        ax_mock.set_xscale.assert_called_once_with(presenter.current_view_props.xscale.lower())
                        ax_mock.autoscale.assert_has_calls([mock.call(True, axis="y")])
                        ax_mock.set_ylabel.assert_called_once_with(presenter.current_view_props.ylabel)
                        ax_mock.set_yscale.assert_called_once_with(presenter.current_view_props.yscale.lower())
                        ax_mock.set_facecolor.assert_called_once_with(presenter.current_view_props.canvas_color)
                        ax_mock.set_xlim.assert_not_called()
                        ax_mock.set_ylim.assert_not_called()

    @mock.patch("mantid.plots.axesfunctions.update_colorplot_datalimits")
    def test_apply_calls_correct_autoscale_for_colorfill_plot(self, mock_colorplot_autoscale):
        """Check that the correct autoscaling function is called for a colorfill plot"""
        # Set images to True to simulate a colorfill plot.
        ax_mock = mock.MagicMock(images=True)
        ax_mock.get_autoscalex_on = mock.Mock(return_value=True)
        ax_mock.get_autoscaley_on = mock.Mock(return_value=True)
        presenter = self._generate_presenter()
        with mock.patch.object(presenter, "get_selected_ax", lambda: ax_mock):
            with mock.patch.object(presenter, "update_view", lambda: None):
                with mock.patch.object(presenter, "axis_changed", lambda: None):
                    presenter.current_view_props = presenter.get_selected_ax_properties()
                    presenter.apply_properties()
                    # update_colorplot_datalimits() should be called once for each axis (x and y)
                    self.assertEqual(2, mock_colorplot_autoscale.call_count)
                    x_call = mock.call(ax_mock, ax_mock.images, axis="x")
                    y_call = mock.call(ax_mock, ax_mock.images, axis="y")
                    mock_colorplot_autoscale.assert_has_calls([x_call, y_call])

    def test_get_axes_names_dict(self):
        actual_dict = get_axes_names_dict(self.fig)
        expected = {"My Axes: (0, 0)": self.fig.get_axes()[0], "(1, 0)": self.fig.get_axes()[1]}
        self.assertEqual(expected, actual_dict)

    def test_get_selected_ax(self):
        presenter = self._generate_presenter()
        self.assertEqual(self.fig.get_axes()[0], presenter.get_selected_ax())

    def test_get_selected_ax_name(self):
        presenter = self._generate_presenter()
        self.assertEqual("My Axes: (0, 0)", presenter.get_selected_ax_name())

    def test_get_selected_ax_properties(self):
        presenter = self._generate_presenter()
        actual_props = presenter.get_selected_ax_properties()
        self.assertIsInstance(actual_props, AxProperties)
        self.assertEqual("My Axes", actual_props.title)
        self.assertEqual("X", actual_props.xlabel)
        self.assertEqual("Linear", actual_props.xscale)
        self.assertEqual("Y", actual_props.ylabel)
        self.assertEqual("Log", actual_props.yscale)
        self.assertEqual(False, actual_props.minor_ticks)
        self.assertEqual(False, actual_props.minor_gridlines)
        self.assertEqual(self.canvas_color, actual_props.canvas_color)

    def test_populate_select_axes_combo_box_called_once_on_construction(self):
        presenter = self._generate_presenter()
        view_mock = presenter.view
        view_mock.populate_select_axes_combo_box.assert_called_once_with(["My Axes: (0, 0)", "(1, 0)"])

    def test_rename_selected_axes_calls_set_selected_axes_selector_text(self):
        presenter = self._generate_presenter()
        new_name = "New Axes Name: (0, 0)"
        presenter.rename_selected_axes(new_name)
        presenter.view.set_selected_axes_selector_text.assert_called_once_with(new_name)

    def test_rename_selected_axes_updates_axes_names_dict(self):
        presenter = self._generate_presenter()
        new_name = "New Axes Name: (0, 0)"
        presenter.rename_selected_axes(new_name)
        self.assertEqual(presenter.axes_names_dict, {new_name: self.fig.get_axes()[0], "(1, 0)": self.fig.get_axes()[1]})

    def test_set_ax_title_sets_title_and_updates_axes_names_dict(self):
        presenter = self._generate_presenter()
        new_title = "New Title"
        ax = self.fig.get_axes()[1]
        presenter.set_ax_title(ax, new_title)
        self.assertIn((new_title + ": (1, 0)", ax), get_axes_names_dict(self.fig).items())
        self.assertEqual(new_title, ax.title.get_text())

    def test_update_view_calls_correct_setters_with_correct_values(self):
        presenter = self._generate_presenter()
        new_view_mock = mock.Mock(get_selected_ax_name=lambda: "My Axes: (0, 0)", get_axis=lambda: "x", get_properties=lambda: {})
        ax = self.fig.get_axes()[0]
        setters = ["set_title", "set_lower_limit", "set_upper_limit", "set_label", "set_scale"]
        expected_vals = [self.title, ax.get_xlim()[0], ax.get_xlim()[1], self.x_label, self.x_scale.capitalize()]
        with mock.patch.object(presenter, "view", new_view_mock):
            presenter.update_view()
            for setter, value in zip(setters, expected_vals):
                getattr(new_view_mock, setter).assert_called_once_with(value)

    def test_autoscale_changed(self):
        mock_view = mock.Mock(
            get_selected_ax_name=lambda: "My Axes: (0, 0)",
            get_axis=lambda: "x",
            get_properties=lambda: {},
            get_autoscale_enabled=mock.Mock(return_value=True),
        )
        presenter = Presenter(self.fig, view=mock_view)
        mock_view.reset_mock()
        presenter.autoscale_changed()
        mock_view.get_autoscale_enabled.assert_called_once_with()
        mock_view.set_limit_input_enabled.assert_called_once_with(False)

    def test_ax_limits_set_correctly(self):
        mock_view = mock.Mock(
            get_lower_limit=mock.Mock(return_value=-1.0),
            get_upper_limit=mock.Mock(return_value=10.0),
            get_selected_ax_name=lambda: "My Axes: (0, 0)",
            get_axis=lambda: "x",
            get_properties=lambda: {},
        )

        presenter = Presenter(self.fig, view=mock_view)
        presenter.get_selected_ax_name = "x"
        presenter.axis_changed()

        self.assertEqual(presenter.current_view_props["xlim"], (-1.0, 10.0))

    def test_apply_all_properties_with_errors_notifies_parent_presenter_on_allowed_exceptions(self):
        mock_view = mock.Mock(get_selected_ax_name=lambda: "My Axes: (0, 0)", get_axis=lambda: "x", get_properties=lambda: {})
        mock_success_callback, mock_error_callback = mock.Mock(), mock.Mock()
        presenter = Presenter(self.fig, view=mock_view, success_callback=mock_success_callback, error_callback=mock_error_callback)

        # Only these exceptions should be caught by the axes tab.
        for exception in [AssertionError, IndexError, KeyError, TypeError, ValueError]:
            presenter.apply_all_properties = mock.Mock(side_effect=exception("This exception should be caught!"))
            presenter._apply_all_properties_with_errors()

            mock_success_callback.assert_not_called()
            mock_error_callback.assert_called()

            presenter.apply_all_properties.reset_mock()
            mock_success_callback.reset_mock()
            mock_error_callback.reset_mock()

    def test_apply_all_properties_with_errors_notifies_parent_presenter_on_success(self):
        mock_view = mock.Mock(get_selected_ax_name=lambda: "My Axes: (0, 0)", get_axis=lambda: "x", get_properties=lambda: {})
        mock_success_callback, mock_error_callback = mock.Mock(), mock.Mock()
        presenter = Presenter(self.fig, view=mock_view, success_callback=mock_success_callback, error_callback=mock_error_callback)
        presenter.apply_all_properties = mock.Mock()

        presenter._apply_all_properties_with_errors()

        mock_success_callback.assert_called()
        mock_error_callback.assert_not_called()


if __name__ == "__main__":
    unittest.main()
