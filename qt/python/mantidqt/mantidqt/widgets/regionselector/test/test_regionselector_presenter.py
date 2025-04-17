# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import call, DEFAULT, Mock, patch, MagicMock

from mantidqt.widgets.regionselector.presenter import RegionSelector
from mantidqt.widgets.sliceviewer.models.workspaceinfo import WS_TYPE


class RegionSelectorTest(unittest.TestCase):
    def setUp(self) -> None:
        self._ws_info_patcher = patch.multiple("mantidqt.widgets.regionselector.presenter", Dimensions=DEFAULT, WorkspaceInfo=DEFAULT)
        self.patched_deps = self._ws_info_patcher.start()
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        self.mock_view = MagicMock()
        self.mock_view.data_view.ax._get_aspect_ratio.return_value = 1

    def tearDown(self) -> None:
        self._ws_info_patcher.stop()

    def test_matrix_workspaces_allowed(self):
        self.assertIsNotNone(RegionSelector(Mock(), view=Mock()))

    def test_show_all_data_not_called_on_creation(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        region_selector.show_all_data_clicked = Mock()

        region_selector.show_all_data_clicked.assert_not_called()

    def test_invalid_workspaces_fail(self):
        invalid_types = [WS_TYPE.MDH, WS_TYPE.MDE, None]
        mock_ws = Mock()

        for ws_type in invalid_types:
            self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = ws_type
            with self.assertRaisesRegex(NotImplementedError, "Matrix Workspaces"):
                RegionSelector(mock_ws, view=Mock())

    def test_creation_without_workspace(self):
        region_selector = RegionSelector(view=Mock())
        self.assertIsNotNone(region_selector)
        self.assertIsNone(region_selector.model.ws)

    def test_update_workspace_updates_model(self):
        region_selector = RegionSelector(view=Mock())
        region_selector.show_all_data_clicked = Mock()
        mock_ws = Mock()

        region_selector.update_workspace(mock_ws)

        self.assertEqual(mock_ws, region_selector.model.ws)

    def test_update_workspace_with_invalid_workspaces_fails(self):
        invalid_types = [WS_TYPE.MDH, WS_TYPE.MDE, None]
        mock_ws = Mock()
        region_selector = RegionSelector(view=Mock())

        for ws_type in invalid_types:
            self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = ws_type
            with self.assertRaisesRegex(NotImplementedError, "Matrix Workspaces"):
                region_selector.update_workspace(mock_ws)

    def test_update_workspace_updates_view(self):
        mock_view = Mock()
        region_selector = RegionSelector(view=mock_view)
        region_selector.show_all_data_clicked = Mock()
        mock_ws = Mock()

        region_selector.update_workspace(mock_ws)

        mock_view.set_workspace.assert_called_once_with(mock_ws)
        region_selector.show_all_data_clicked.assert_called_once()

    def test_add_rectangular_region_creates_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=self.mock_view)

        region_selector.add_rectangular_region("test", "black", "/")

        self.assertEqual(1, len(region_selector._selectors))
        self.assertTrue(region_selector._selectors[0].active)
        self.assertEqual("test", region_selector._selectors[0].region_type())

    def test_add_second_rectangular_region_deactivates_first_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=self.mock_view)

        region_selector.add_rectangular_region("test", "black", "/")
        region_selector._drawing_region = False
        region_selector.add_rectangular_region("test", "black", "/")

        self.assertEqual(2, len(region_selector._selectors))

        self.assertFalse(region_selector._selectors[0].active)
        self.assertTrue(region_selector._selectors[1].active)

    def test_clear_workspace_will_clear_all_the_selectors_and_model_workspace(self):
        region_selector = RegionSelector(view=self.mock_view)
        region_selector.show_all_data_clicked = Mock()
        mock_ws = Mock()

        region_selector.update_workspace(mock_ws)
        region_selector.add_rectangular_region("test", "black", "/")
        region_selector.add_rectangular_region("test", "black", "/")

        region_selector.clear_workspace()

        self.assertEqual(0, len(region_selector._selectors))
        self.assertEqual(None, region_selector.model.ws)

    def test_clear_workspace_will_clear_the_figure_in_the_view(self):
        mock_view = Mock()
        region_selector = RegionSelector(view=mock_view)

        region_selector.clear_workspace()

        mock_view.clear_figure.assert_called_once_with()

    def test_get_region_with_two_signal_regions(self):
        region_selector, selector_one, selector_two = self._mock_selectors()

        region = region_selector.get_region("signal")
        self.assertEqual(4, len(region))
        self.assertEqual([selector_one.extents[2], selector_one.extents[3], selector_two.extents[2], selector_two.extents[3]], region)

    def test_get_region_with_different_region_types(self):
        region_selector, selector_one, selector_two = self._mock_selectors("signal", "background")
        region = region_selector.get_region("signal")
        self.assertEqual(2, len(region))
        self.assertEqual([selector_one.extents[2], selector_one.extents[3]], region)

    def test_canvas_clicked_does_nothing_when_redrawing_region(self):
        region_selector, selector_one, selector_two = self._mock_selectors()

        region_selector._drawing_region = True

        region_selector.canvas_clicked(Mock())

        selector_one.set_active.assert_not_called()
        selector_two.set_active.assert_not_called()

    def test_canvas_clicked_sets_selectors_inactive(self):
        region_selector, selector_one, selector_two = self._mock_selectors()

        region_selector._contains_point = Mock(return_value=False)

        region_selector.canvas_clicked(Mock())

        selector_one.set_active.assert_called_once_with(False)
        selector_two.set_active.assert_called_once_with(False)

    def test_canvas_clicked_sets_selectors_active_if_contains_point(self):
        region_selector, selector_one, selector_two = self._mock_selectors()

        event = Mock()
        event.xdata = 1.5
        event.ydata = 3.5
        region_selector.canvas_clicked(event)

        selector_one.set_active.assert_has_calls([call(False), call(True)])
        selector_two.set_active.assert_called_once_with(False)

    def test_canvas_clicked_sets_only_one_selector_active_if_multiple_contain_point(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_two.extents = selector_one.extents

        event = Mock()
        event.xdata = 1.5
        event.ydata = 3.5
        region_selector.canvas_clicked(event)

        selector_one.set_active.assert_has_calls([call(False), call(True)])
        selector_two.set_active.assert_called_once_with(False)

    def test_delete_key_pressed_will_do_nothing_if_no_selectors_exist(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())

        event = Mock()
        event.key = "delete"

        self.assertEqual(0, len(region_selector._selectors))
        region_selector.key_pressed(event)
        self.assertEqual(0, len(region_selector._selectors))

    def test_delete_key_pressed_will_do_nothing_if_no_selectors_are_active(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_one.active = False
        selector_two.active = False

        event = Mock()
        event.key = "delete"

        self.assertEqual(2, len(region_selector._selectors))
        region_selector.key_pressed(event)
        self.assertEqual(2, len(region_selector._selectors))

    def test_delete_key_pressed_will_remove_the_active_selector(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_one.active = False
        selector_two.active = True
        selector_two.artists = []

        event = Mock()
        event.key = "delete"

        self.assertEqual(2, len(region_selector._selectors))
        region_selector.key_pressed(event)
        self.assertEqual(1, len(region_selector._selectors))

        selector_two.set_active.assert_called_once_with(False)
        selector_two.update.assert_called_once_with()

    def test_delete_key_pressed_will_notify_region_changed(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_one.active, selector_two.active = False, True
        selector_two.artists = []
        mock_observer = Mock()
        region_selector.subscribe(mock_observer)

        event = Mock()
        event.key = "delete"

        region_selector.key_pressed(event)

        mock_observer.notifyRegionChanged.assert_called_once()

    def test_mouse_moved_will_not_set_override_cursor_if_no_selectors_exist(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        region_selector.view.set_override_cursor = Mock()

        event = Mock()
        event.xdata, event.ydata = 1.0, 2.0

        region_selector.mouse_moved(event)

        region_selector.view.set_override_cursor.assert_called_once_with(False)

    def test_mouse_moved_will_not_set_override_cursor_if_no_selectors_are_active(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_one.active = False
        selector_two.active = False

        event = Mock()
        event.xdata, event.ydata = 5.5, 7.5

        region_selector.mouse_moved(event)

        region_selector.view.set_override_cursor.assert_called_once_with(False)

    def test_mouse_moved_will_not_set_override_cursor_if_not_hovering_over_selector(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_one.active = False
        selector_two.active = True

        event = Mock()
        event.xdata, event.ydata = 1.5, 3.5

        region_selector.mouse_moved(event)

        region_selector.view.set_override_cursor.assert_called_once_with(False)

    def test_mouse_moved_will_set_override_cursor_if_hovering_over_active_selector(self):
        region_selector, selector_one, selector_two = self._mock_selectors()
        selector_one.active = False
        selector_two.active = True

        event = Mock()
        event.xdata, event.ydata = 5.5, 7.5

        region_selector.mouse_moved(event)

        region_selector.view.set_override_cursor.assert_called_once_with(True)

    def test_on_rectangle_selected_notifies_observer(self):
        region_selector = RegionSelector(ws=Mock(), view=self.mock_view)
        mock_observer = Mock()
        region_selector.subscribe(mock_observer)

        region_selector.add_rectangular_region("test", "black", "/")
        region_selector._on_rectangle_selected(Mock(), Mock())

        mock_observer.notifyRegionChanged.assert_called_once()

    def test_cancel_drawing_region_will_remove_last_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=self.mock_view)
        region_selector.add_rectangular_region("test", "black", "/")
        self.assertEqual(1, len(region_selector._selectors))
        region_selector.cancel_drawing_region()
        self.assertEqual(0, len(region_selector._selectors))

    def test_when_multiple_region_adds_are_requested_only_one_region_is_added(self):
        # Given
        region_selector = RegionSelector(ws=Mock(), view=self.mock_view)
        region_selector.add_rectangular_region("test", "black", "/")
        self.assertEqual(1, len(region_selector._selectors))

        # When
        region_selector.add_rectangular_region("test2", "green", "O")

        # Then
        self.assertEqual(1, len(region_selector._selectors))
        self.assertEqual(region_selector._selectors[0]._region_type, "test2")

    def test_cancel_drawing_region_with_no_selectors_does_not_crash(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        region_selector.cancel_drawing_region()

    def test_display_rectangular_region_creates_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=self._mock_view_with_axes_limits((2000, 10000), (0, 500)))
        # The expected extents should be within the axes x and y limits
        expected_extents = (4000, 6000, 100, 300)
        region_type = "test"

        region_selector.display_rectangular_region(region_type, "black", "/", expected_extents[2], expected_extents[3])

        self.assertEqual(1, len(region_selector._selectors))
        self._check_rectangular_region(region_selector._selectors[0], region_type, expected_extents)

    def test_display_rectangular_region_y1_out_of_bounds_does_not_add_selector(self):
        y_limits = (200, 500)
        region_selector = RegionSelector(ws=Mock(), view=self._mock_view_with_axes_limits((2000, 10000), y_limits))

        region_selector.display_rectangular_region("test", "black", "/", y_limits[0] - 50, y_limits[1])

        self.assertEqual(0, len(region_selector._selectors))

    def test_display_rectangular_region_y2_out_of_bounds_does_not_add_selector(self):
        y_limits = (200, 500)
        region_selector = RegionSelector(ws=Mock(), view=self._mock_view_with_axes_limits((2000, 10000), y_limits))

        region_selector.display_rectangular_region("test", "black", "/", y_limits[0], y_limits[1] + 50)

        self.assertEqual(0, len(region_selector._selectors))

    def test_display_rectangular_region_does_not_add_duplicate_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=self._mock_view_with_axes_limits((2000, 10000), (0, 500)))
        # The expected extents should be within the axes x and y limits
        expected_extents = (4000, 6000, 100, 300)
        region_type = "test"
        color = "black"
        hatch = "/"

        region_selector.display_rectangular_region(region_type, color, hatch, expected_extents[2], expected_extents[3])
        self.assertEqual(1, len(region_selector._selectors))
        region_selector.display_rectangular_region(region_type, color, hatch, expected_extents[2], expected_extents[3])

        self.assertEqual(1, len(region_selector._selectors))
        self._check_rectangular_region(region_selector._selectors[0], region_type, expected_extents)

    def test_display_rectangular_region_adds_second_different_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=self._mock_view_with_axes_limits((2000, 10000), (0, 500)))
        # The expected extents should be within the axes x and y limits
        expected_x_values = (4000, 6000)
        first_y_values = (100, 200)
        second_y_values = (300, 400)
        region_type = "test"
        color = "black"
        hatch = "/"

        region_selector.display_rectangular_region(region_type, color, hatch, first_y_values[0], first_y_values[1])
        region_selector.display_rectangular_region(region_type, color, hatch, second_y_values[0], second_y_values[1])

        self.assertEqual(2, len(region_selector._selectors))
        for i, expected_y_values in enumerate([first_y_values, second_y_values]):
            self._check_rectangular_region(region_selector._selectors[i], region_type, expected_x_values + expected_y_values)

    @staticmethod
    def _mock_selectors(selector_one_type="signal", selector_two_type="signal"):
        selector_one, selector_two = Mock(), Mock()
        selector_one.region_type = Mock(return_value=selector_one_type)
        selector_two.region_type = Mock(return_value=selector_two_type)
        selector_one.set_active, selector_two.set_active = Mock(), Mock()
        selector_one.extents = [1, 2, 3, 4]
        selector_two.extents = [5, 6, 7, 8]

        region_selector = RegionSelector(ws=Mock(), view=Mock())
        region_selector._selectors.append(selector_one)
        region_selector._selectors.append(selector_two)

        return region_selector, selector_one, selector_two

    @staticmethod
    def _mock_view_with_axes_limits(x_limits: tuple[float, float], y_limits: tuple[float, float]):
        mock_view = Mock()
        mock_view.data_view.ax.get_xbound.return_value = mock_view.data_view.ax.get_xlim.return_value = x_limits
        mock_view.data_view.ax.get_ybound.return_value = mock_view.data_view.ax.get_ylim.return_value = y_limits
        mock_view.data_view.ax._get_aspect_ratio.return_value = 1
        return mock_view

    def _check_rectangular_region(
        self, selector: RegionSelector, expected_region_type: str, expected_extents: tuple[float, float, float, float], is_active=False
    ):
        self.assertEqual(is_active, selector.active)
        self.assertEqual(expected_region_type, selector.region_type())
        self.assertTupleEqual(expected_extents, selector.extents)


if __name__ == "__main__":
    unittest.main()
