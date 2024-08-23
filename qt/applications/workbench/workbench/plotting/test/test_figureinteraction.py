# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#


# system imports
import unittest

# third-party library imports
import matplotlib

matplotlib.use("AGG")
import matplotlib.pyplot as plt
import numpy as np
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QMenu
from numpy.testing import assert_almost_equal

# local package imports
from mantid.plots import MantidAxes
from unittest import mock
from unittest.mock import MagicMock, PropertyMock, call, patch
from mantid.simpleapi import CreateWorkspace
from mantidqt.plotting.figuretype import FigureType
from mantidqt.plotting.functions import plot, pcolormesh_from_names, plot_contour, pcolormesh
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.figureinteraction import FigureInteraction, LogNorm


@start_qapplication
class FigureInteractionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ws = CreateWorkspace(
            DataX=np.array([10, 20, 30], dtype=np.float64),
            DataY=np.array([2, 3], dtype=np.float64),
            DataE=np.array([0.02, 0.02], dtype=np.float64),
            Distribution=False,
            UnitX="Wavelength",
            YUnitLabel="Counts",
            OutputWorkspace="ws",
        )
        cls.ws1 = CreateWorkspace(
            DataX=np.array([11, 21, 31], dtype=np.float64),
            DataY=np.array([3, 4], dtype=np.float64),
            DataE=np.array([0.03, 0.03], dtype=np.float64),
            Distribution=False,
            UnitX="Wavelength",
            YUnitLabel="Counts",
            OutputWorkspace="ws1",
        )
        # initialises the QApplication
        super(cls, FigureInteractionTest).setUpClass()

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()
        cls.ws1.delete()

    def setUp(self):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        fig_manager.fit_browser.tool = None
        self.interactor = FigureInteraction(fig_manager)
        self.fig, self.ax = plt.subplots()  # type: matplotlib.figure.Figure, MantidAxes

    def tearDown(self):
        plt.close("all")
        del self.fig
        del self.ax
        del self.interactor

    # Success tests
    def test_construction_registers_handler_for_button_press_event(self):
        fig_manager = MagicMock()
        fig_manager.canvas = MagicMock()
        interactor = FigureInteraction(fig_manager)
        expected_call = [
            call("button_press_event", interactor.on_mouse_button_press),
            call("button_release_event", interactor.on_mouse_button_release),
            call("draw_event", interactor.draw_callback),
            call("motion_notify_event", interactor.motion_event),
            call("resize_event", interactor.mpl_redraw_annotations),
            call("figure_leave_event", interactor.on_leave),
            call("scroll_event", interactor.on_scroll),
            call("key_press_event", interactor.on_key_press),
        ]
        fig_manager.canvas.mpl_connect.assert_has_calls(expected_call)
        self.assertEqual(len(expected_call), fig_manager.canvas.mpl_connect.call_count)

    def test_disconnect_called_for_each_registered_handler(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        fig_manager.canvas = canvas
        interactor = FigureInteraction(fig_manager)
        interactor.disconnect()
        self.assertEqual(interactor.nevents, canvas.mpl_disconnect.call_count)

    @patch("workbench.plotting.figureinteraction.QMenu", autospec=True)
    @patch("workbench.plotting.figureinteraction.figure_type", autospec=True)
    def test_right_click_gives_no_context_menu_for_empty_figure(self, mocked_figure_type, mocked_qmenu):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_right_click()
        mocked_figure_type.return_value = FigureType.Empty

        with patch.object(interactor.toolbar_manager, "is_tool_active", lambda: False):
            interactor.on_mouse_button_press(mouse_event)
            self.assertEqual(0, mocked_qmenu.call_count)

    @patch("workbench.plotting.figureinteraction.QMenu", autospec=True)
    @patch("workbench.plotting.figureinteraction.figure_type", autospec=True)
    def test_right_click_gives_context_menu_for_color_plot(self, mocked_figure_type, mocked_qmenu):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_right_click()
        mocked_figure_type.return_value = FigureType.Image

        # Expect a call to QMenu() for the outer menu followed by three more calls
        # for the Axes, Normalization and Colorbar menus
        qmenu_call1 = MagicMock()
        qmenu_call2 = MagicMock()
        qmenu_call3 = MagicMock()
        qmenu_call4 = MagicMock()
        mocked_qmenu.side_effect = [qmenu_call1, qmenu_call2, qmenu_call3, qmenu_call4]

        with patch("workbench.plotting.figureinteraction.QActionGroup", autospec=True):
            with patch.object(interactor.toolbar_manager, "is_tool_active", lambda: False):
                interactor.on_mouse_button_press(mouse_event)
                self.assertEqual(0, qmenu_call1.addAction.call_count)
                expected_qmenu_calls = [
                    call(),
                    call("Axes", qmenu_call1),
                    call("Normalization", qmenu_call1),
                    call("Color bar", qmenu_call1),
                ]
                self.assertEqual(expected_qmenu_calls, mocked_qmenu.call_args_list)
                # 4 actions in Axes submenu
                self.assertEqual(4, qmenu_call2.addAction.call_count)
                # 2 actions in Normalization submenu
                self.assertEqual(2, qmenu_call3.addAction.call_count)
                # 2 actions in Colorbar submenu
                self.assertEqual(2, qmenu_call4.addAction.call_count)

    @patch("workbench.plotting.figureinteraction.QMenu", autospec=True)
    @patch("workbench.plotting.figureinteraction.figure_type", autospec=True)
    def test_right_click_gives_context_menu_for_plot_without_fit_enabled(self, mocked_figure_type, mocked_qmenu_cls):
        fig_manager = self._create_mock_fig_manager_to_accept_right_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_right_click()
        mouse_event.inaxes.get_xlim.return_value = (1, 2)
        mouse_event.inaxes.get_ylim.return_value = (1, 2)
        mouse_event.inaxes.lines = []
        mocked_figure_type.return_value = FigureType.Line

        # Expect a call to QMenu() for the outer menu followed by two more calls
        # for the Axes, Normalization, and Markers menus
        outer_qmenu_call = MagicMock()
        qmenu_call2 = MagicMock()
        qmenu_call3 = MagicMock()
        qmenu_call4 = MagicMock()
        mocked_qmenu_cls.side_effect = [outer_qmenu_call, qmenu_call2, qmenu_call3, qmenu_call4]

        with patch("workbench.plotting.figureinteraction.QActionGroup", autospec=True):
            with patch("workbench.plotting.figureinteraction.QAction"):
                with patch.object(interactor.toolbar_manager, "is_tool_active", lambda: False):
                    with patch.object(interactor, "add_error_bars_menu", MagicMock()):
                        interactor.on_mouse_button_press(mouse_event)
                        self.assertEqual(0, outer_qmenu_call.addSeparator.call_count)
                        self.assertEqual(1, outer_qmenu_call.addAction.call_count)  # Show/hide legend action
                        expected_qmenu_calls = [
                            call(),
                            call("Axes", outer_qmenu_call),
                            call("Normalization", outer_qmenu_call),
                            call("Markers", outer_qmenu_call),
                        ]
                        self.assertEqual(expected_qmenu_calls, mocked_qmenu_cls.call_args_list)
                        # 4 actions in Axes submenu
                        self.assertEqual(4, qmenu_call2.addAction.call_count)
                        # 2 actions in Normalization submenu
                        self.assertEqual(2, qmenu_call3.addAction.call_count)
                        # 3 actions in Markers submenu
                        self.assertEqual(3, qmenu_call4.addAction.call_count)

    def test_toggle_normalization_no_errorbars(self):
        self._test_toggle_normalization(errorbars_on=False, plot_kwargs={"distribution": True})

    def test_toggle_normalization_with_errorbars(self):
        self._test_toggle_normalization(errorbars_on=True, plot_kwargs={"distribution": True})

    def test_correct_yunit_label_when_overplotting_after_normalization_toggle(self):
        # The earlier version of Matplotlib on RHEL throws an error when performing the second
        # plot in this test, if the lines have errorbars. The error occurred when it attempted
        # to draw an interactive legend. Plotting without errors still fulfills the purpose of this
        # test, so turn them off for old Matplotlib versions.
        errors = True
        if int(matplotlib.__version__[0]) < 2:
            errors = False

        fig = plot([self.ws], spectrum_nums=[1], errors=errors, plot_kwargs={"distribution": True})
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)

        ax = fig.axes[0]
        fig_interactor._toggle_normalization(ax)
        self.assertEqual(r"Counts ($\AA$)$^{-1}$", ax.get_ylabel())
        plot([self.ws1], spectrum_nums=[1], errors=errors, overplot=True, fig=fig)
        self.assertEqual(r"Counts ($\AA$)$^{-1}$", ax.get_ylabel())

    def test_normalization_toggle_with_no_autoscale_on_update_no_errors(self):
        self.ax.autoscale(enable=False, axis="both")
        self._test_toggle_normalization(errorbars_on=False, plot_kwargs={"distribution": True, "autoscale_on_update": False})

    def test_normalization_toggle_with_no_autoscale_on_update_with_errors(self):
        self.ax.autoscale(enable=False, axis="both")
        self._test_toggle_normalization(errorbars_on=True, plot_kwargs={"distribution": True, "autoscale_on_update": False})

    def test_add_error_bars_menu(self):
        self.ax.errorbar([0, 15000], [0, 14000], yerr=[10, 10000], label="MyLabel 2")
        self.ax.containers[0][2][0].axes.creation_args = [{"errorevery": 1}]
        main_menu = QMenu()
        self.interactor.add_error_bars_menu(main_menu, self.ax)

        # Check the expected sub-menu with buttons is added
        added_menu = main_menu.children()[1]
        self.assertTrue(any(FigureInteraction.SHOW_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))
        self.assertTrue(any(FigureInteraction.HIDE_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))

    def test_context_menu_not_added_for_scripted_plot_without_errors(self):
        self.ax.plot([0, 15000], [0, 15000], label="MyLabel")
        self.ax.plot([0, 15000], [0, 14000], label="MyLabel 2")

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        # plot above doesn't have errors, nor is a MantidAxes
        # so no context menu will be added
        self.interactor.add_error_bars_menu(main_menu, self.ax)

        # number of children should remain unchanged
        self.assertEqual(1, len(main_menu.children()))

    def test_scripted_plot_line_without_label_handled_properly(self):
        # having the special nolabel is usually present on lines with errors,
        # but sometimes can be present on lines without errors, this test covers that case
        self.ax.plot([0, 15000], [0, 15000], label="_nolegend_")
        self.ax.plot([0, 15000], [0, 15000], label="_nolegend_")

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        # plot above doesn't have errors, nor is a MantidAxes
        # so no context menu will be added for error bars
        self.interactor.add_error_bars_menu(main_menu, self.ax)

        # number of children should remain unchanged
        self.assertEqual(1, len(main_menu.children()))

    def test_context_menu_added_for_scripted_plot_with_errors(self):
        self.ax.plot([0, 15000], [0, 15000], label="MyLabel")
        self.ax.errorbar([0, 15000], [0, 14000], yerr=[10, 10000], label="MyLabel 2")
        self.ax.containers[0][2][0].axes.creation_args = [{"errorevery": 1}]

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        # plot above doesn't have errors, nor is a MantidAxes
        # so no context menu will be added
        self.interactor.add_error_bars_menu(main_menu, self.ax)

        added_menu = main_menu.children()[1]

        # actions should have been added now, which for this case are only `Show all` and `Hide all`
        self.assertTrue(any(FigureInteraction.SHOW_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))
        self.assertTrue(any(FigureInteraction.HIDE_ERROR_BARS_BUTTON_TEXT == child.text() for child in added_menu.children()))

    def test_context_menu_includes_plot_type_if_plot_has_multiple_lines(self):
        fig, self.ax = plt.subplots(subplot_kw={"projection": "mantid"})
        self.ax.plot([0, 1], [0, 1])
        self.ax.plot([0, 1], [0, 1])

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        self.interactor._add_plot_type_option_menu(main_menu, self.ax)

        added_menu = main_menu.children()[1]
        self.assertEqual(added_menu.children()[0].text(), "Plot Type")

    def test_context_menu_does_not_include_plot_type_if_plot_has_one_line(self):
        fig, self.ax = plt.subplots(subplot_kw={"projection": "mantid"})
        self.ax.errorbar([0, 1], [0, 1], capsize=1)

        main_menu = QMenu()
        # QMenu always seems to have 1 child when empty,
        # but just making sure the count as expected at this point in the test
        self.assertEqual(1, len(main_menu.children()))

        self.interactor._add_plot_type_option_menu(main_menu, self.ax)

        # Number of children should remain unchanged
        self.assertEqual(1, len(main_menu.children()))

    def test_scripted_plot_show_and_hide_all(self):
        self.ax.plot([0, 15000], [0, 15000], label="MyLabel")
        self.ax.errorbar([0, 15000], [0, 14000], yerr=[10, 10000], label="MyLabel 2")
        self.ax.containers[0][2][0].axes.creation_args = [{"errorevery": 1}]

        anonymous_menu = QMenu()
        # this initialises some of the class internals
        self.interactor.add_error_bars_menu(anonymous_menu, self.ax)

        self.assertTrue(self.ax.containers[0][2][0].get_visible())
        self.interactor.errors_manager.toggle_all_errors(self.ax, make_visible=False)
        self.assertFalse(self.ax.containers[0][2][0].get_visible())

        # make the menu again, this updates the internal state of the errors manager
        # and is what actually happens when the user opens the menu again
        self.interactor.add_error_bars_menu(anonymous_menu, self.ax)
        self.interactor.errors_manager.toggle_all_errors(self.ax, make_visible=True)
        self.assertTrue(self.ax.containers[0][2][0].get_visible())

    def test_no_normalisation_options_on_non_workspace_plot(self):
        fig, self.ax = plt.subplots(subplot_kw={"projection": "mantid"})
        self.ax.plot([1, 2], [1, 2], label="myLabel")

        anonymous_menu = QMenu()
        self.assertEqual(None, self.interactor._add_normalization_option_menu(anonymous_menu, self.ax))

    # Failure tests
    def test_construction_with_non_qt_canvas_raises_exception(self):
        class NotQtCanvas(object):
            pass

        class FigureManager(object):
            def __init__(self):
                self.canvas = NotQtCanvas()

        self.assertRaises(RuntimeError, FigureInteraction, FigureManager())

    def test_context_menu_change_axis_scale_is_axis_aware(self):
        fig = plot([self.ws, self.ws1], spectrum_nums=[1, 1], tiled=True)
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)
        scale_types = ("log", "log")

        ax = fig.axes[0]
        ax1 = fig.axes[1]
        current_scale_types = (ax.get_xscale(), ax.get_yscale())
        current_scale_types1 = (ax1.get_xscale(), ax1.get_yscale())
        self.assertEqual(current_scale_types, current_scale_types1)

        fig_interactor._quick_change_axes(scale_types, ax)
        current_scale_types2 = (ax.get_xscale(), ax.get_yscale())
        self.assertNotEqual(current_scale_types2, current_scale_types1)

    def test_scale_on_ragged_workspaces_maintained_when_toggling_normalisation(self):
        ws = CreateWorkspace(DataX=[1, 2, 3, 4, 2, 4, 6, 8], DataY=[2] * 8, NSpec=2, OutputWorkspace="ragged_ws")
        fig = pcolormesh_from_names([ws])
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)
        fig_interactor._toggle_normalization(fig.axes[0])

        clim = fig.axes[0].images[0].get_clim()
        fig_interactor._toggle_normalization(fig.axes[0])
        self.assertEqual(clim, fig.axes[0].images[0].get_clim())
        self.assertNotEqual((-0.1, 0.1), fig.axes[0].images[0].get_clim())

    def test_log_maintained_when_normalisation_toggled(self):
        ws = CreateWorkspace(DataX=[1, 2, 3, 4, 2, 4, 6, 8], DataY=[2] * 8, NSpec=2, OutputWorkspace="ragged_ws")
        fig = pcolormesh_from_names([ws])
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)
        fig_interactor._change_colorbar_axes(LogNorm)

        fig_interactor._toggle_normalization(fig.axes[0])

        self.assertTrue(isinstance(fig.axes[0].images[-1].norm, LogNorm))

    @patch("workbench.plotting.figureinteraction.QMenu", autospec=True)
    @patch("workbench.plotting.figureinteraction.figure_type", autospec=True)
    def test_right_click_gives_marker_menu_when_hovering_over_one(self, mocked_figure_type, mocked_qmenu_cls):
        mouse_event = self._create_mock_right_click()
        mouse_event.inaxes.get_xlim.return_value = (1, 2)
        mouse_event.inaxes.get_ylim.return_value = (1, 2)
        mocked_figure_type.return_value = FigureType.Line
        marker1 = MagicMock()
        marker2 = MagicMock()
        marker3 = MagicMock()
        self.interactor.markers = [marker1, marker2, marker3]
        for marker in self.interactor.markers:
            marker.is_above.return_value = True

        # Expect a call to QMenu() for the outer menu followed by two more calls
        # for the Axes and Normalization menus
        qmenu_call1 = MagicMock()
        qmenu_call2 = MagicMock()
        qmenu_call3 = MagicMock()
        qmenu_call4 = MagicMock()
        mocked_qmenu_cls.side_effect = [qmenu_call1, qmenu_call2, qmenu_call3, qmenu_call4]

        with patch("workbench.plotting.figureinteraction.QActionGroup", autospec=True):
            with patch.object(self.interactor.toolbar_manager, "is_tool_active", lambda: False):
                with patch.object(self.interactor, "add_error_bars_menu", MagicMock()):
                    self.interactor.on_mouse_button_press(mouse_event)
                    self.assertEqual(0, qmenu_call1.addSeparator.call_count)
                    self.assertEqual(0, qmenu_call1.addAction.call_count)
                    expected_qmenu_calls = [
                        call(),
                        call(marker1.name, qmenu_call1),
                        call(marker2.name, qmenu_call1),
                        call(marker3.name, qmenu_call1),
                    ]
                    self.assertEqual(expected_qmenu_calls, mocked_qmenu_cls.call_args_list)
                    # 2 Actions in marker menu
                    self.assertEqual(2, qmenu_call2.addAction.call_count)
                    self.assertEqual(2, qmenu_call3.addAction.call_count)
                    self.assertEqual(2, qmenu_call4.addAction.call_count)

    @patch("workbench.plotting.figureinteraction.SingleMarker")
    def test_adding_horizontal_marker_adds_correct_marker(self, mock_marker):
        y0, y1 = 0, 1
        data = MagicMock()
        axis = MagicMock()
        self.interactor._add_horizontal_marker(data, y0, y1, axis)
        expected_call = call(
            self.interactor.canvas, "#2ca02c", data, y0, y1, name="marker 0", marker_type="YSingle", line_style="dashed", axis=axis
        )

        self.assertEqual(1, mock_marker.call_count)
        mock_marker.assert_has_calls([expected_call])

    @patch("workbench.plotting.figureinteraction.SingleMarker")
    def test_adding_vertical_marker_adds_correct_marker(self, mock_marker):
        x0, x1 = 0, 1
        data = MagicMock()
        axis = MagicMock()
        self.interactor._add_vertical_marker(data, x0, x1, axis)
        expected_call = call(
            self.interactor.canvas, "#2ca02c", data, x0, x1, name="marker 0", marker_type="XSingle", line_style="dashed", axis=axis
        )

        self.assertEqual(1, mock_marker.call_count)
        mock_marker.assert_has_calls([expected_call])

    def test_delete_marker_does_not_delete_markers_if_not_present(self):
        marker = MagicMock()
        self.interactor.markers = []

        self.interactor._delete_marker(marker)

        self.assertEqual(0, self.interactor.canvas.draw.call_count)
        self.assertEqual(0, marker.marker.remove.call_count)
        self.assertEqual(0, marker.remove_all_annotations.call_count)

    def test_delete_marker_preforms_correct_cleanup(self):
        marker = MagicMock()
        self.interactor.markers = [marker]

        self.interactor._delete_marker(marker)

        self.assertEqual(1, marker.marker.remove.call_count)
        self.assertEqual(1, marker.remove_all_annotations.call_count)
        self.assertEqual(1, self.interactor.canvas.draw.call_count)
        self.assertNotIn(marker, self.interactor.markers)

    @patch("workbench.plotting.figureinteraction.SingleMarkerEditor")
    @patch("workbench.plotting.figureinteraction.QApplication")
    def test_edit_marker_opens_correct_editor(self, mock_qapp, mock_editor):
        marker = MagicMock()
        expected_call = [call(self.interactor.canvas, marker, self.interactor.valid_lines, self.interactor.valid_colors, [])]

        self.interactor._edit_marker(marker)

        self.assertEqual(1, mock_qapp.restoreOverrideCursor.call_count)
        mock_editor.assert_has_calls(expected_call)

    @patch("workbench.plotting.figureinteraction.GlobalMarkerEditor")
    def test_global_edit_marker_opens_correct_editor(self, mock_editor):
        marker = MagicMock()
        self.interactor.markers = [marker]
        expected_call = [call(self.interactor.canvas, [marker], self.interactor.valid_lines, self.interactor.valid_colors)]

        self.interactor._global_edit_markers()

        mock_editor.assert_has_calls(expected_call)

    def test_motion_event_returns_if_toolbar_has_active_tools(self):
        self.interactor.toolbar_manager.is_tool_active = MagicMock(return_value=True)
        self.interactor._set_hover_cursor = MagicMock()
        self.interactor.motion_event(MagicMock())

        self.assertEqual(0, self.interactor._set_hover_cursor.call_count)

    def test_motion_event_returns_if_fit_active(self):
        self.interactor.toolbar_manager.is_fit_active = MagicMock(return_value=True)
        self.interactor._set_hover_cursor = MagicMock()
        self.interactor.motion_event(MagicMock())

        self.assertEqual(0, self.interactor._set_hover_cursor.call_count)

    def test_motion_event_changes_cursor_and_draws_canvas_if_any_marker_is_moving(self):
        markers = [MagicMock(), MagicMock(), MagicMock()]
        for marker in markers:
            marker.mouse_move.return_value = True
        event = MagicMock()
        event.xdata = 1
        event.ydata = 2
        self.interactor.markers = markers
        self.interactor.toolbar_manager.is_tool_active = MagicMock(return_value=False)
        self.interactor.toolbar_manager.is_fit_active = MagicMock(return_value=False)
        self.interactor._set_hover_cursor = MagicMock()

        self.interactor.motion_event(event)

        self.interactor._set_hover_cursor.assert_has_calls([call(1, 2)])
        self.assertEqual(1, self.interactor.canvas.draw.call_count)

    def test_motion_event_changes_cursor_and_does_not_draw_canvas_if_no_marker_is_moving(self):
        markers = [MagicMock(), MagicMock(), MagicMock()]
        for marker in markers:
            marker.mouse_move.return_value = False
        event = MagicMock()
        event.xdata = 1
        event.ydata = 2
        self.interactor.markers = markers
        self.interactor.toolbar_manager.is_tool_active = MagicMock(return_value=False)
        self.interactor.toolbar_manager.is_fit_active = MagicMock(return_value=False)
        self.interactor._set_hover_cursor = MagicMock()

        self.interactor.motion_event(event)

        self.interactor._set_hover_cursor.assert_has_calls([call(1, 2)])
        self.assertEqual(0, self.interactor.canvas.draw.call_count)

    def test_redraw_annotations_removes_and_adds_all_annotations_for_all_markers(self):
        markers = [MagicMock(), MagicMock(), MagicMock()]
        call_list = [call.remove_all_annotations(), call.add_all_annotations()]
        self.interactor.markers = markers
        self.interactor.redraw_annotations()

        for marker in markers:
            marker.assert_has_calls(call_list)

    def test_mpl_redraw_annotations_does_not_redraw_if_event_does_not_have_a_button_attribute(self):
        self.interactor.redraw_annotations = MagicMock()
        event = MagicMock(spec="no_button")
        event.no_button = MagicMock(spec="no_button")

        self.interactor.mpl_redraw_annotations(event.no_button)
        self.assertEqual(0, self.interactor.redraw_annotations.call_count)

    def test_mpl_redraw_annotations_does_not_redraw_if_event_button_not_pressed(self):
        self.interactor.redraw_annotations = MagicMock()
        event = MagicMock()
        event.button = None
        self.interactor.mpl_redraw_annotations(event)
        self.assertEqual(0, self.interactor.redraw_annotations.call_count)

    def test_mpl_redraw_annotations_redraws_if_button_pressed(self):
        self.interactor.redraw_annotations = MagicMock()
        event = MagicMock()
        self.interactor.mpl_redraw_annotations(event)
        self.assertEqual(1, self.interactor.redraw_annotations.call_count)

    def test_toggle_normalisation_on_contour_plot_maintains_contour_line_colour(self):
        from mantid.plots.utility import convert_color_to_hex

        ws = CreateWorkspace(DataX=[1, 2, 3, 4, 2, 4, 6, 8], DataY=[2] * 8, NSpec=2, OutputWorkspace="test_ws")
        fig = plot_contour([ws])

        for col in fig.get_axes()[0].collections:
            col.set_color("#ff9900")

        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)
        fig_interactor._toggle_normalization(fig.axes[0])

        self.assertTrue(all(convert_color_to_hex(col.get_edgecolor()[0]) == "#ff9900" for col in fig.get_axes()[0].collections))

    def test_toggle_normalisation_applies_to_all_images_if_one_colorbar(self):
        fig = pcolormesh([self.ws, self.ws])

        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)

        # there should be 3 axes, 2 colorplots and 1 colorbar
        self.assertEqual(3, len(fig.axes))
        fig.axes[0].tracked_workspaces.values()
        self.assertTrue(fig.axes[0].tracked_workspaces["ws"][0].is_normalized)
        self.assertTrue(fig.axes[1].tracked_workspaces["ws"][0].is_normalized)

        fig_interactor._toggle_normalization(fig.axes[0])

        self.assertFalse(fig.axes[0].tracked_workspaces["ws"][0].is_normalized)
        self.assertFalse(fig.axes[1].tracked_workspaces["ws"][0].is_normalized)

    def test_marker_annotations_are_removed_and_redrawn_on_scroll_zoom(self):
        event = MagicMock()
        event.inaxes = MagicMock()
        event.inaxes.lines = ["line"]

        self.interactor.redraw_annotations = MagicMock()
        self.interactor.on_scroll(event)
        self.interactor.redraw_annotations.assert_called_once()

    def test_marker_annotations_are_removed_on_middle_mouse_pan_begin(self):
        fig_manager = self._create_mock_fig_manager_to_accept_middle_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_middle_click()

        interactor.toolbar_manager.is_tool_active = MagicMock(return_value=False)
        interactor.toolbar_manager.is_zoom_active = MagicMock(return_value=False)

        interactor._remove_all_marker_annotations = MagicMock()
        interactor.on_mouse_button_press(mouse_event)
        interactor._remove_all_marker_annotations.assert_called_once()

    def test_marker_annotations_are_added_on_middle_mouse_pan_end(self):
        fig_manager = self._create_mock_fig_manager_to_accept_middle_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_middle_click()

        interactor.toolbar_manager.is_tool_active = MagicMock(return_value=False)

        interactor._add_all_marker_annotations = MagicMock()
        interactor.on_mouse_button_release(mouse_event)
        interactor._add_all_marker_annotations.assert_called_once()

    def test_open_double_click_dialog_called_on_mouse_release(self):
        """
        The logic for opening dialogs by double-clicking is inside the mouse release callback.
        """
        fig_manager = self._create_mock_fig_manager_to_accept_left_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_double_left_click()

        interactor._open_double_click_dialog = MagicMock()
        interactor.on_mouse_button_press(mouse_event)
        interactor.on_mouse_button_release(event=MagicMock())

        interactor._open_double_click_dialog.assert_called_once()

    def test_open_double_click_dialog_not_called_on_mouse_release_when_no_double_click(self):
        """
        The function that opens the settings dialogs after a double click should not
        be called if there wasn't a double click.
        """
        fig_manager = self._create_mock_fig_manager_to_accept_left_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)

        interactor._open_double_click_dialog = MagicMock()
        interactor.on_mouse_button_release(event=MagicMock())

        interactor._open_double_click_dialog.assert_not_called()

    def test_double_left_click_calls_show_axis_editor(self):
        fig_manager = self._create_mock_fig_manager_to_accept_left_click()
        fig_manager.fit_browser.tool = None
        interactor = FigureInteraction(fig_manager)
        mouse_event = self._create_mock_double_left_click()

        interactor._show_axis_editor = MagicMock()
        interactor.on_mouse_button_press(mouse_event)
        interactor.on_mouse_button_release(event=MagicMock())
        interactor._show_axis_editor.assert_called_once()

    @mock.patch("workbench.plotting.figureinteraction.XAxisEditor")
    def test_click_x_axes_launches_x_axes_editor(self, mock_x_editor):
        self.interactor.canvas.figure = MagicMock()
        axes = self._create_axes_for_axes_editor_test(mouse_over="xaxis")
        self.interactor.canvas.figure.get_axes.return_value = axes
        self.interactor._show_axis_editor(event=MagicMock())

        mock_x_editor.assert_called_once()

    @mock.patch("workbench.plotting.figureinteraction.YAxisEditor")
    def test_click_y_axes_launches_y_axes_editor(self, mock_y_editor):
        self.interactor.canvas.figure = MagicMock()
        axes = self._create_axes_for_axes_editor_test(mouse_over="yaxis")
        self.interactor.canvas.figure.get_axes.return_value = axes
        self.interactor._show_axis_editor(event=MagicMock())

        mock_y_editor.assert_called_once()

    @mock.patch("workbench.plotting.figureinteraction.XAxisEditor")
    def test_click_x_axes_tick_label_launches_x_axes_editor(self, mock_x_editor):
        self.interactor.canvas.figure = MagicMock()
        axes = self._create_axes_for_axes_editor_test(mouse_over="xaxis_tick")
        self.interactor.canvas.figure.get_axes.return_value = axes
        self.interactor._show_axis_editor(event=MagicMock())

        mock_x_editor.assert_called_once()

    @mock.patch("workbench.plotting.figureinteraction.YAxisEditor")
    def test_click_y_axes_tick_label_launches_y_axes_editor(self, mock_y_editor):
        self.interactor.canvas.figure = MagicMock()
        axes = self._create_axes_for_axes_editor_test(mouse_over="yaxis_tick")
        self.interactor.canvas.figure.get_axes.return_value = axes
        self.interactor._show_axis_editor(event=MagicMock())

        mock_y_editor.assert_called_once()

    def test_keyboard_shortcut_switch_x_scale(self):
        key_press_event = self._create_mock_key_press_event("k")
        key_press_event.inaxes.get_xscale.return_value = "linear"
        key_press_event.inaxes.get_yscale.return_value = "log"
        key_press_event.inaxes.get_xlim.return_value = (0, 100)
        key_press_event.inaxes.get_ylim.return_value = (5, 10)
        key_press_event.inaxes.get_lines.return_value = ["fake_line"]
        fig_manager = MagicMock()
        fig_manager.canvas = MagicMock()
        interactor = FigureInteraction(fig_manager)
        interactor.on_key_press(key_press_event)
        key_press_event.inaxes.set_xscale.assert_called_once_with("log")
        key_press_event.inaxes.set_yscale.assert_called_once_with("log")
        key_press_event.inaxes.set_xlim.assert_called_once_with((0, 100))
        key_press_event.inaxes.set_ylim.assert_called_once_with((5, 10))

    def test_keyboard_shortcut_switch_y_scale(self):
        key_press_event = self._create_mock_key_press_event("l")
        key_press_event.inaxes.get_xscale.return_value = "linear"
        key_press_event.inaxes.get_yscale.return_value = "log"
        key_press_event.inaxes.get_xlim.return_value = (0, 100)
        key_press_event.inaxes.get_ylim.return_value = (5, 10)
        key_press_event.inaxes.get_lines.return_value = ["fake_line"]
        fig_manager = MagicMock()
        fig_manager.canvas = MagicMock()
        interactor = FigureInteraction(fig_manager)
        interactor.on_key_press(key_press_event)
        key_press_event.inaxes.set_xscale.assert_called_once_with("linear")
        key_press_event.inaxes.set_yscale.assert_called_once_with("linear")
        key_press_event.inaxes.set_xlim.assert_called_once_with((0, 100))
        key_press_event.inaxes.set_ylim.assert_called_once_with((5, 10))

    def test_legend_not_picked_up_from_scroll_event(self):
        event = self._create_mock_scroll_event(button="up")
        type(event.inaxes).lines = PropertyMock(return_value=["fake_line"])
        legend = MagicMock()
        legend.get_draggable.return_value = True
        legend.contains.return_value = True
        event.inaxes.axes.get_legend.return_value = legend
        self.interactor.on_scroll(event)
        calls = [call(False, False, "loc"), call(True, False, "loc")]
        legend.set_draggable.assert_has_calls(calls)

    # Private methods
    def _create_mock_fig_manager_to_accept_right_click(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        type(canvas).buttond = PropertyMock(return_value={Qt.RightButton: 3})
        fig_manager.canvas = canvas
        return fig_manager

    def _create_mock_fig_manager_to_accept_middle_click(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        type(canvas).buttond = PropertyMock(return_value={Qt.MiddleButton: 4})
        fig_manager.canvas = canvas
        return fig_manager

    def _create_mock_fig_manager_to_accept_left_click(self):
        fig_manager = MagicMock()
        canvas = MagicMock()
        type(canvas).buttond = PropertyMock(return_value={Qt.LeftButton: 1})
        fig_manager.canvas = canvas
        return fig_manager

    def _create_mock_right_click(self):
        mouse_event = MagicMock(inaxes=MagicMock(spec=MantidAxes, collections=[], creation_args=[{}]))
        type(mouse_event).button = PropertyMock(return_value=3)
        return mouse_event

    def _create_mock_middle_click(self):
        mouse_event = MagicMock(inaxes=MagicMock(spec=MantidAxes, collections=[], creation_args=[{}]))
        type(mouse_event).button = PropertyMock(return_value=4)
        return mouse_event

    def _create_mock_double_left_click(self) -> object:
        mouse_event = MagicMock(inaxes=MagicMock(spec=MantidAxes, collections=[], creation_args=[{}]))
        type(mouse_event).button = PropertyMock(return_value=1)
        type(mouse_event).dblclick = PropertyMock(return_value=True)
        return mouse_event

    def _create_mock_key_press_event(self, key):
        key_press_event = MagicMock(inaxes=MagicMock(spec=MantidAxes, collections=[], creation_args=[{}]))
        type(key_press_event).key = PropertyMock(return_value=key)
        return key_press_event

    def _create_mock_scroll_event(self, button):
        event = MagicMock(inaxes=MagicMock(spec=MantidAxes, collections=[], creation_args=[{}]))
        type(event).button = MagicMock(return_value=button)
        return event

    def _create_axes_for_axes_editor_test(self, mouse_over: str):
        ax = MagicMock()
        ax.xaxis.contains.return_value = (mouse_over == "xaxis", {})
        ax.yaxis.contains.return_value = (mouse_over == "yaxis", {})
        ax.title.contains.return_value = (False, {})
        ax.xaxis.label.contains.return_value = (False, {})
        ax.yaxis.label.contains.return_value = (False, {})

        xtick = MagicMock()
        ytick = MagicMock()
        xtick.contains.return_value = (mouse_over == "xaxis_tick", {})
        ytick.contains.return_value = (mouse_over == "yaxis_tick", {})
        ax.get_xticklabels.return_value = [xtick]
        ax.get_yticklabels.return_value = [ytick]
        return [ax]

    def _test_toggle_normalization(self, errorbars_on, plot_kwargs):
        fig = plot([self.ws], spectrum_nums=[1], errors=errorbars_on, plot_kwargs=plot_kwargs)
        mock_canvas = MagicMock(figure=fig)
        fig_manager_mock = MagicMock(canvas=mock_canvas)
        fig_interactor = FigureInteraction(fig_manager_mock)

        # Earlier versions of matplotlib do not store the data assciated with a
        # line with high precision and hence we need to set a lower tolerance
        # when making comparisons of this data
        if matplotlib.__version__ < "2":
            decimal_tol = 1
        else:
            decimal_tol = 7

        ax = fig.axes[0]
        fig_interactor._toggle_normalization(ax)
        assert_almost_equal(ax.lines[0].get_xdata(), [15, 25])
        assert_almost_equal(ax.lines[0].get_ydata(), [0.2, 0.3], decimal=decimal_tol)
        self.assertEqual("Counts ($\\AA$)$^{-1}$", ax.get_ylabel())
        fig_interactor._toggle_normalization(ax)
        assert_almost_equal(ax.lines[0].get_xdata(), [15, 25])
        assert_almost_equal(ax.lines[0].get_ydata(), [2, 3], decimal=decimal_tol)
        self.assertEqual("Counts", ax.get_ylabel())


if __name__ == "__main__":
    unittest.main()
