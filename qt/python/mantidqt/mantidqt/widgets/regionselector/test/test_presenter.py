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


if __name__ == '__main__':
    unittest.main()
