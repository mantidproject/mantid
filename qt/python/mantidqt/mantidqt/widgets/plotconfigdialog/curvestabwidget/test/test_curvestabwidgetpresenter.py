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
from matplotlib.pyplot import figure, subplots
from numpy import array_equal

from mantid.simpleapi import CreateWorkspace
from mantid.plots import datafunctions
from mantid.plots.utility import convert_color_to_hex, MantidAxType
from unittest.mock import Mock, patch, call
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import CurvesTabWidgetPresenter, remove_curve_from_ax, curve_has_errors
from mantidqt.widgets.plotconfigdialog.legendtabwidget.presenter import LegendTabWidgetPresenter


class CurvesTabWidgetPresenterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fig = figure()
        cls.ws = CreateWorkspace(DataX=[0, 1], DataY=[3, 4], DataE=[0.1, 0.1], NSpec=1, UnitX="TOF", OutputWorkspace="ws")
        cls.ax0 = cls.fig.add_subplot(221, projection="mantid")
        cls.ax0.set_title("Axes 0")
        cls.curve0 = cls.ax0.errorbar(cls.ws, specNum=1, fmt=":g", marker=1, label="Workspace")
        cls.ax0.plot([0, 1], [2, 3], "--r", marker="v", lw=1.1, label="noerrors")

        ax1 = cls.fig.add_subplot(222)
        ax1.set_title("Image")
        ax1.imshow([[0, 1], [0, 1]])

        cls.ax2 = cls.fig.add_subplot(223)
        cls.ax2.errorbar([0, 1], [10, 11], yerr=[0.1, 0.2], label="Errorbars")

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()

    def _get_no_errors_line(self):
        for line in self.ax0.lines:
            if line.get_label() == "noerrors":
                return line

    def _generate_presenter(self, mock_view=None, fig=None, current_curve_name="Workspace"):
        if not mock_view:
            mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: current_curve_name)
        if not fig:
            fig = self.fig
        mock_view.select_curve_list = Mock(selectedItems=lambda: [])
        return CurvesTabWidgetPresenter(fig=fig, view=mock_view)

    def test_axes_names_dict_gets_axes_on_init_(self):
        presenter = self._generate_presenter()
        self.assertIn("Axes 0: (0, 0)", presenter.axes_names_dict)
        self.assertNotIn("Image: (1, 0)", presenter.axes_names_dict)

    def test_populate_select_axes_combobox_called_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.populate_select_axes_combo_box.assert_called_once_with(["Axes 0: (0, 0)", "(1, 0)"])

    def test_populate_select_curve_list_called_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.populate_select_curve_list.assert_called_once_with(["Workspace", "noerrors"])

    def test_update_view_called_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.update_fields.assert_called_once_with(CurveProperties.from_curve(self.curve0))

    def test_curve_names_not_equal_in_curve_names_dict_if_curves_have_same_labels(self):
        line = self._get_no_errors_line()
        # Patch line in ax0 to have same label as the errorbar curve in the
        # same axes
        with patch.object(line, "get_label", lambda: "Workspace"):
            presenter = self._generate_presenter()
        self.assertSequenceEqual(["Workspace", "Workspace (1)"], sorted(presenter.curve_names_dict.keys()))

    def test_curves_with__no_legend__label_not_in_curves_name_dict(self):
        line = self.fig.get_axes()[0].get_lines()[1]
        with patch.object(line, "get_label", lambda: "_nolegend_"):
            presenter = self._generate_presenter()
        self.assertNotIn(line, presenter.curve_names_dict.values())

    def test_updating_curve_label_updates_curve_names_dict(self):
        presenter = self._generate_presenter()
        line = self._get_no_errors_line()
        err_bars = self.fig.get_axes()[0].containers[0]
        presenter.set_new_curve_name_in_dict_and_list(err_bars, "new_label")
        self.assertEqual({"new_label": err_bars, "noerrors": line}, presenter.curve_names_dict)

    def test_remove_curve_from_mantid_ax_with_workspace_artist(self):
        mantid_ax = self.ax0
        ws_curve = mantid_ax.errorbar(self.ws)
        remove_curve_from_ax(ws_curve)
        self.assertNotIn(ws_curve[0], mantid_ax.lines)
        self.assertNotIn(ws_curve, mantid_ax.containers)
        tracked_artists = []
        for ws_artists_list in mantid_ax.tracked_workspaces.values():
            for ws_artists in ws_artists_list:
                for artist in ws_artists._artists:
                    tracked_artists.append(artist)
        self.assertNotIn(ws_curve, tracked_artists)

    def test_remove_curve_from_mantid_ax_with_non_workspace_artist(self):
        mantid_ax = self.ax0
        curve = mantid_ax.plot([0], [0])[0]
        remove_curve_from_ax(curve)
        self.assertNotIn(curve, mantid_ax.lines)

    def test_remove_curve_from_mpl_axes(self):
        ax = self.ax2
        line = ax.plot([0], [0])[0]
        err_cont = ax.errorbar([0], [0], [1])
        remove_curve_from_ax(line)
        self.assertNotIn(line, ax.lines)
        remove_curve_from_ax(err_cont)
        self.assertNotIn(err_cont[0], ax.lines)
        self.assertNotIn(err_cont, ax.containers)

    def test_remove_current_curve_removes_curve_from_curves_names_and_list(self):
        self.ax2.plot([0], [0], label="new_line")
        mock_view = Mock(
            get_selected_ax_name=lambda: "(1, 0)", get_current_curve_name=lambda: "new_line", get_selected_curves_names=lambda: ["new_line"]
        )
        presenter = self._generate_presenter(mock_view=mock_view)
        with patch.object(presenter, "update_view", lambda: None):
            presenter.remove_selected_curves()
        self.assertNotIn("new_line", presenter.curve_names_dict)
        presenter.view.remove_select_curve_list_selected_items.assert_called_once_with()

    def test_axes_removed_from_axes_names_dict_when_all_curves_removed(self):
        fig = figure()
        ax0 = fig.add_subplot(211)
        ax0.set_title("First Axes")
        ax1 = fig.add_subplot(212)
        ax1.set_title("Second Axes")
        ax0.plot([0, 1], [0, 1], label="ax0 curve")
        ax1.plot([0], [0], label="ax1 curve")
        mock_view = Mock(
            get_selected_ax_name=lambda: "First Axes: (0, 0)",
            get_current_curve_name=lambda: "ax0 curve",
            get_selected_curves_names=lambda: ["ax0 curve"],
        )
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        with patch.object(presenter, "update_view", lambda: None):
            presenter.remove_selected_curves()
        self.assertNotIn("First Axes", presenter.axes_names_dict)
        self.assertNotIn("ax0 curve", presenter.curve_names_dict)

    def test_curves_and_legend_tabs_close_when_all_curves_removed(self):
        fig, ax = subplots()
        ax.plot([[0], [0]], label="line1")
        ax.legend()

        mock_view = Mock(
            get_selected_ax_name=lambda: "(0, 0)", get_current_curve_name=lambda: "line1", get_selected_curves_names=lambda: ["line1"]
        )
        mock_view.select_curve_list = Mock(selectedItems=lambda: [], count=lambda: 0)
        mock_view.select_axes_combo_box.count = Mock(return_value=0)

        parent_presenter_mock = Mock()

        legend_presenter = LegendTabWidgetPresenter(fig=fig, view=mock_view, parent_presenter=parent_presenter_mock)
        curves_presenter = CurvesTabWidgetPresenter(
            fig=fig, view=mock_view, parent_presenter=parent_presenter_mock, legend_tab=legend_presenter
        )

        curves_presenter.remove_selected_curves()

        curves_presenter.parent_presenter.forget_tab_from_presenter.assert_has_calls([call(curves_presenter), call(legend_presenter)])
        mock_view.close.assert_called()

    def test_curve_has_errors_on_workspace_with_no_errors(self):
        try:
            ws = CreateWorkspace(DataX=[0], DataY=[0], NSpec=1, OutputWorkspace="test_ws")
            fig = figure()
            ax = fig.add_subplot(111, projection="mantid")
            curve = ax.plot(ws, specNum=1)[0]
            self.assertFalse(curve_has_errors(curve))
        finally:
            ws.delete()

    def test_curve_has_errors_on_workspace_with_errors(self):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        curve = ax.plot(self.ws, specNum=1)[0]
        self.assertTrue(curve_has_errors(curve))

    def test_curve_has_errors_returns_false_on_bin_plot_workspace_with_no_errors(self):
        ws = CreateWorkspace(DataX=[0, 1], DataY=[0, 1], NSpec=2, OutputWorkspace="test_ws")
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        curve = ax.plot(ws, wkspIndex=0, axis=MantidAxType.BIN)[0]
        self.assertFalse(curve_has_errors(curve))
        ws.delete()

    def test_curve_has_errors_returns_true_on_bin_plot_workspace_with_errors(self):
        ws = CreateWorkspace(DataX=[0, 1], DataY=[0, 1], DataE=[0.1, 0.1], NSpec=2, OutputWorkspace="test_ws")
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        curve = ax.plot(ws, wkspIndex=0, axis=MantidAxType.BIN)[0]
        self.assertTrue(curve_has_errors(curve))
        ws.delete()

    def test_replot_selected_curve(self):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        ax.set_title("Axes 0")
        ax.plot(self.ws, specNum=1, label="Workspace")
        presenter = self._generate_presenter(fig=fig)
        presenter.view.select_curve_list.currentIndex.return_value = 0
        new_plot_kwargs = {"errorevery": 2, "linestyle": "-.", "color": "r", "marker": "v"}
        presenter._replot_current_curve(new_plot_kwargs)
        new_err_container = presenter.get_current_curve()
        self.assertEqual(new_err_container[0].get_linestyle(), "-.")
        self.assertEqual(new_err_container[0].get_color(), "r")
        self.assertEqual(new_err_container[0].get_marker(), "v")
        # Test only one errorbar is plotted
        self.assertEqual(1, len(new_err_container[2][0].get_segments()))

    def test_replot_second_curve_twice_does_not_alter_line_order(self):
        fig = self.make_figure_with_multiple_curves()
        ax = fig.get_axes()[0]
        presenter = self._generate_presenter(fig=fig, current_curve_name="Workspace 2")
        presenter.view.select_curve_list.currentIndex.return_value = 1
        new_plot_kwargs = {"errorevery": 2, "linestyle": "-.", "color": "r", "marker": "v"}
        presenter._replot_current_curve(new_plot_kwargs)
        second_curve = presenter.get_current_curve()[0]
        self.assertEqual(ax.get_lines().index(second_curve), 1)
        new_plot_kwargs = {"errorevery": 2, "linestyle": "-.", "color": "b", "marker": "v"}
        presenter._replot_current_curve(new_plot_kwargs)
        second_curve = presenter.get_current_curve()[0]
        self.assertEqual(ax.get_lines().index(second_curve), 1)

    def test_curve_errorbars_are_hidden_on_apply_properties_when_hide_curve_is_ticked(self):
        fig = figure()
        ax = fig.add_subplot(111)
        ax.errorbar([0, 1, 2, 4], [0, 1, 2, 4], yerr=[0.1, 0.2, 0.3, 0.4], label="errorbar_plot")
        ax.containers[0][2][0].axes.creation_args = [{"errorevery": 1}]
        mock_view_props = Mock(get_plot_kwargs=lambda: {"visible": False}, hide_errors=False, hide=True, __getitem__=lambda s, x: False)
        mock_view = Mock(
            get_selected_ax_name=lambda: "(0, 0)", get_current_curve_name=lambda: "errorbar_plot", get_properties=lambda: mock_view_props
        )
        mock_view.select_curve_list.currentIndex.return_value = 0
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        presenter.apply_properties()
        self.assertFalse(ax.containers[0][2][0].get_visible())

    def test_selecting_and_applying_errorbar_curves(self):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        ax.errorbar(self.ws, specNum=1, label="Workspace", marker=".", markersize=4.0, capsize=1.0)
        ax.errorbar(self.ws, specNum=1, label="Workspace 2", marker=".", markersize=4.0, capsize=1.0)
        mock_view_props = Mock(get_plot_kwargs=lambda: {}, __getitem__=lambda s, x: False)
        mock_view = Mock(
            get_selected_ax_name=lambda: "(0, 0)", get_current_curve_name=lambda: "Workspace 2", get_properties=lambda: mock_view_props
        )
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        presenter.apply_properties()

    def make_figure_with_multiple_curves(self):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        ax.set_title("Axes 0")
        ax.plot(self.ws, specNum=1, label="Workspace")
        ax.plot(self.ws, specNum=1, label="Workspace 2")
        ax.plot(self.ws, specNum=1, label="Workspace 3")
        return fig

    @patch.object(CurvesTabWidgetPresenter, "apply_properties")
    def test_line_apply_to_all_button_sets_and_applies_properties_to_each_curve(self, mock_apply_properties):
        fig = self.make_figure_with_multiple_curves()

        presenter = self._generate_presenter(fig=fig)
        presenter.line_apply_to_all()

        self.assertEqual(presenter.view.line.set_style.call_count, 3)
        self.assertEqual(presenter.view.line.set_draw_style.call_count, 3)
        self.assertEqual(presenter.view.line.set_width.call_count, 3)
        self.assertEqual(mock_apply_properties.call_count, 3)

    @patch.object(CurvesTabWidgetPresenter, "apply_properties")
    def test_marker_apply_to_all_button_sets_and_applies_properties_to_each_curve(self, mock_apply_properties):
        fig = self.make_figure_with_multiple_curves()

        presenter = self._generate_presenter(fig=fig)
        presenter.marker_apply_to_all()

        self.assertEqual(presenter.view.marker.set_style.call_count, 3)
        self.assertEqual(presenter.view.marker.set_size.call_count, 3)
        self.assertEqual(mock_apply_properties.call_count, 3)

    @patch.object(CurvesTabWidgetPresenter, "apply_properties")
    def test_errorbar_apply_to_all_button_sets_and_applies_properties_to_each_curve_if_hide_errorbars_is_unticked(
        self, mock_apply_properties
    ):
        fig = self.make_figure_with_multiple_curves()

        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")

        mock_view.errorbars.get_hide.return_value = False

        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        presenter.errorbars_apply_to_all()

        self.assertEqual(presenter.view.errorbars.set_hide.call_count, 3)
        self.assertEqual(presenter.view.errorbars.set_width.call_count, 3)
        self.assertEqual(presenter.view.errorbars.set_capsize.call_count, 3)
        self.assertEqual(presenter.view.errorbars.set_cap_thickness.call_count, 3)
        self.assertEqual(presenter.view.errorbars.set_error_every.call_count, 3)
        self.assertEqual(mock_apply_properties.call_count, 3)

    @patch.object(CurvesTabWidgetPresenter, "apply_properties")
    def test_errorbar_apply_to_all_button_does_not_set_properties_if_hide_errorbars_is_ticked(self, mock_apply_properties):
        fig = self.make_figure_with_multiple_curves()

        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")

        mock_view.errorbars.get_hide.return_value = True

        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        presenter.errorbars_apply_to_all()

        self.assertEqual(presenter.view.errorbars.set_hide.call_count, 3)
        self.assertEqual(presenter.view.errorbars.set_width.call_count, 0)
        self.assertEqual(presenter.view.errorbars.set_capsize.call_count, 0)
        self.assertEqual(presenter.view.errorbars.set_cap_thickness.call_count, 0)
        self.assertEqual(presenter.view.errorbars.set_error_every.call_count, 0)
        self.assertEqual(mock_apply_properties.call_count, 3)

    def test_hiding_a_curve_on_a_waterfall_plot_also_hides_its_filled_area(self):
        fig = self.make_figure_with_multiple_curves()

        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")

        ax = fig.get_axes()[0]
        ax.set_waterfall(True)
        ax.set_waterfall_fill(True)

        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)

        new_plot_kwargs = {"visible": False}
        presenter._replot_current_curve(new_plot_kwargs)

        self.assertEqual(datafunctions.get_waterfall_fill_for_curve(ax, 0).get_visible(), False)

    def test_changing_line_colour_on_a_waterfall_plot_with_filled_areas_changes_fill_colour_to_match(self):
        fig = self.make_figure_with_multiple_curves()

        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")

        ax = fig.get_axes()[0]
        ax.lines[0].set_color("#ff9900")
        ax.lines[1].set_color("#008fff")
        ax.lines[2].set_color("#42ff00")

        # Create waterfall plot and add filled areas.
        ax.set_waterfall(True)
        ax.set_waterfall_fill(True)

        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        # Change the colour of one of the lines.
        new_plot_kwargs = {"color": "#ffff00"}
        presenter._replot_current_curve(new_plot_kwargs)

        # The fill for that line should be the new colour.
        self.assertEqual(convert_color_to_hex(ax.collections[0].get_facecolor()[0]), ax.lines[0].get_color())

    def test_adding_errorbars_to_waterfall_plot_maintains_waterfall(self):
        fig = self.make_figure_with_multiple_curves()

        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")

        ax = fig.get_axes()[0]
        # Create waterfall plot
        ax.set_waterfall(True)

        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        # Add errobars to the first two lines
        for i in range(2):
            if i == 1:
                presenter.view.get_current_curve_name = lambda: "Workspace 2"
            new_plot_kwargs = {"capsize": 2}
            presenter._replot_current_curve(new_plot_kwargs)

        # Check the errorbar lines and the errorbar cap lines are different.
        # (They would be the same if it was a non-waterfall plot)
        self.assertFalse(array_equal(ax.containers[0][2][0].get_segments(), ax.containers[1][2][0].get_segments()))
        self.assertFalse(array_equal(ax.containers[0][1][0].get_data(), ax.containers[1][1][0].get_data()))

    def test_selecting_many_curves_disables_curve_config(self):
        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")
        presenter = self._generate_presenter(fig=None, mock_view=mock_view)
        mock_view.select_curve_list.selectedItems = lambda: ["item1", "item2"]
        presenter.on_curves_selection_changed()

        mock_view.enable_curve_config.assert_called_with(False)

    def test_selection_one_curve_enables_curve_config(self):
        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")
        presenter = self._generate_presenter(fig=None, mock_view=mock_view)
        mock_view.select_curve_list.selectedItems = lambda: ["item1"]
        presenter.on_curves_selection_changed()

        mock_view.enable_curve_config.assert_called_with(True)

    def make_figure_with_error_bars(self):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        ax.errorbar([0, 1, 2, 4], [0, 1, 2, 4], yerr=[0.1, 0.2, 0.3, 0.4], label="errorbar_plot", errorevery=7)
        ax.set_title("Axes 0")
        ax.plot(self.ws, specNum=1, label="Workspace")
        ax.plot(self.ws, specNum=1, label="Workspace 2")
        ax.plot(self.ws, specNum=1, label="Workspace 3")
        return fig

    def test_errorevery_applied_correctly(self):
        fig = self.make_figure_with_error_bars()
        new_props = CurveProperties(
            {"capsize": 1, "errorevery": 7, "hide": False, "marker": None, "label": "Workspace", "hide_errors": False}
        )
        mock_view = Mock(
            get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace", get_properties=lambda: new_props
        )
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        presenter.apply_properties()
        presenter.update_view()
        with patch.object(presenter.view, "udpate_fields"):
            args, kwargs = presenter.view.update_fields.call_args
            self.assertEqual(args[0].errorevery, 7)

    def test_set_axes_from_object_when_multiple_axes_exist_on_fig(self):
        fig, axes = subplots(2, subplot_kw={"projection": "mantid"})
        axes[0].plot(self.ws, specNum=1, label="Workspace")
        axes[1].plot(self.ws, specNum=1, label="Workspace")
        mock_view = Mock(get_selected_ax_name=lambda: "(0, 0)", get_current_curve_name=lambda: "Workspace")
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)

        presenter.set_axes_from_object(axes[1])

        # ax1 should be index 1 in the axes combo
        mock_view.select_axes_combo_box.setCurrentIndex.assert_called_with(1)

    def test_set_axes_from_object_raises_error_when_axes_not_found(self):
        fig, axes = subplots(2, subplot_kw={"projection": "mantid"})
        axes[0].plot(self.ws, specNum=1, label="Workspace")
        axes[1].plot(self.ws, specNum=1, label="Workspace")
        mock_view = Mock(get_selected_ax_name=lambda: "(0, 0)", get_current_curve_name=lambda: "Workspace")
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)

        with self.assertRaises(ValueError):
            presenter.set_axes_from_object(None)

    def test_set_curve_from_object_when_multiple_curves_exist_on_fig(self):
        fig = self.make_figure_with_multiple_curves()
        ax0 = fig.get_axes()[0]
        curve1 = ax0.lines[1]
        mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)", get_current_curve_name=lambda: "Workspace")
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)

        presenter.set_curve_from_object(curve1)

        # line should be index 1 in the curves list
        mock_view.select_curve_list.setCurrentRow.assert_called_with(1)

    def test_set_axes_from_object_raises_error_when_curve_not_found(self):
        fig, axes = subplots(2, subplot_kw={"projection": "mantid"})
        axes[0].plot(self.ws, specNum=1, label="Workspace")
        mock_view = Mock(get_selected_ax_name=lambda: "(0, 0)", get_current_curve_name=lambda: "Workspace")
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)

        with self.assertRaises(ValueError):
            presenter.set_curve_from_object(None)


if __name__ == "__main__":
    unittest.main()
