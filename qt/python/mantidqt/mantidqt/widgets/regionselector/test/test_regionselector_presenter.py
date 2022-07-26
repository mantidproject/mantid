# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import call, DEFAULT, Mock, patch

from mantidqt.widgets.regionselector.presenter import RegionSelector
from mantidqt.widgets.sliceviewer.models.workspaceinfo import WS_TYPE


class RegionSelectorTest(unittest.TestCase):
    def setUp(self) -> None:
        self._ws_info_patcher = patch.multiple("mantidqt.widgets.regionselector.presenter",
                                               Dimensions=DEFAULT,
                                               WorkspaceInfo=DEFAULT)
        self.patched_deps = self._ws_info_patcher.start()
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX

    def tearDown(self) -> None:
        self._ws_info_patcher.stop()

    def test_matrix_workspaces_allowed(self):
        self.assertIsNotNone(RegionSelector(Mock(), view=Mock()))

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
        mock_ws = Mock()

        region_selector.update_workspace(mock_ws)

        mock_view.set_workspace.assert_called_once_with(mock_ws)

    def test_add_rectangular_region_creates_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())

        region_selector.add_rectangular_region("test")

        self.assertEqual(1, len(region_selector._selectors))
        self.assertTrue(region_selector._selectors[0].active)
        self.assertEqual("test", region_selector._selectors[0].region_type())

    def test_add_second_rectangular_region_deactivates_first_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())

        region_selector.add_rectangular_region("test")
        region_selector.add_rectangular_region("test")

        self.assertEqual(2, len(region_selector._selectors))

        self.assertFalse(region_selector._selectors[0].active)
        self.assertTrue(region_selector._selectors[1].active)

    def test_get_region_with_two_signal_regions(self):
        region_selector, selector_one, selector_two = self._mock_selectors()

        region = region_selector.get_region("signal")
        self.assertEqual(4, len(region))
        self.assertEqual([selector_one.extents[2], selector_one.extents[3], selector_two.extents[2],
                          selector_two.extents[3]], region)

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

    def test_on_rectangle_selected_notifies_observer(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        mock_observer = Mock()
        region_selector.subscribe(mock_observer)

        region_selector.add_rectangular_region("test")
        region_selector._on_rectangle_selected(Mock(), Mock())

        mock_observer.notifyRegionChanged.assert_called_once()

    def test_cancel_drawing_region_will_remove_last_selector(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        region_selector.add_rectangular_region("test")
        self.assertEqual(1, len(region_selector._selectors))
        region_selector.cancel_drawing_region()
        self.assertEqual(0, len(region_selector._selectors))

    def test_cancel_drawing_region_with_no_selectors_does_not_crash(self):
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        region_selector.cancel_drawing_region()

    def _mock_selectors(self, selector_one_type="signal", selector_two_type="signal"):
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


if __name__ == '__main__':
    unittest.main()
