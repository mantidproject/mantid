# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import Mock, patch, DEFAULT

from mantidqt.widgets.regionselector.presenter import RegionSelector
from mantidqt.widgets.sliceviewer.models.workspaceinfo import WS_TYPE


class RegionSelectorTest(unittest.TestCase):
    def setUp(self) -> None:
        self._ws_info_patcher = patch.multiple("mantidqt.widgets.regionselector.presenter",
                                               Dimensions=DEFAULT,
                                               WorkspaceInfo=DEFAULT)
        self.patched_deps = self._ws_info_patcher.start()

    def tearDown(self) -> None:
        self._ws_info_patcher.stop()

    def test_matrix_workspaces_allowed(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX

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
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX

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
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX

        region_selector.update_workspace(mock_ws)

        mock_view.set_workspace.assert_called_once_with(mock_ws)

    def test_add_rectangular_region_creates_selector(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        region_selector = RegionSelector(ws=Mock(), view=Mock())

        region_selector.add_rectangular_region()

        self.assertIsNotNone(region_selector._selector)

    def test_on_rectangle_selected_notifies_observer(self):
        self.patched_deps["WorkspaceInfo"].get_ws_type.return_value = WS_TYPE.MATRIX
        region_selector = RegionSelector(ws=Mock(), view=Mock())
        mock_observer = Mock()
        region_selector.subscribe(mock_observer)

        region_selector.add_rectangular_region()
        region_selector._on_rectangle_selected(Mock(), Mock())

        mock_observer.notifyRegionChanged.assert_called_once()


if __name__ == '__main__':
    unittest.main()
