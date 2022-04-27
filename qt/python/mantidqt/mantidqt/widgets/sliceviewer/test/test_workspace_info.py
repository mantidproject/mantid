# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
import unittest
from unittest import mock

from mantid.api import MatrixWorkspace, MultipleExperimentInfos
from mantidqt.widgets.sliceviewer.models.workspaceinfo import WorkspaceInfo, WS_TYPE


class TestWorkspaceInfo(unittest.TestCase):
    def test_get_ws_type_with_MatrixWorkspace(self):
        mock_ws = mock.Mock(spec=MatrixWorkspace)
        self.assertEqual(WS_TYPE.MATRIX, WorkspaceInfo.get_ws_type(mock_ws))

    def test_get_ws_type_with_MDHistoWorkspace(self):
        mock_ws = mock.Mock(spec=MultipleExperimentInfos)
        mock_ws.isMDHistoWorkspace = mock.Mock(return_value=True)
        self.assertEqual(WS_TYPE.MDH, WorkspaceInfo.get_ws_type(mock_ws))

    def test_get_ws_type_with_MDEventWorkspace(self):
        mock_ws = mock.Mock(spec=MultipleExperimentInfos)
        mock_ws.isMDHistoWorkspace = mock.Mock(return_value=False)
        self.assertEqual(WS_TYPE.MDE, WorkspaceInfo.get_ws_type(mock_ws))

    def test_get_ws_type_raises_with_unexpected_type(self):
        mock_ws = mock.Mock()
        self.assertRaisesRegex(ValueError, "Unsupported", WorkspaceInfo.get_ws_type, mock_ws)

    def test_get_display_indices_with_two_dimensions(self):
        indices = WorkspaceInfo.display_indices(slicepoint=(None, 0, None), transpose=False)
        self.assertEqual(indices, (0, 2))

        indices = WorkspaceInfo.display_indices(slicepoint=(None, None, 0), transpose=False)
        self.assertEqual(indices, (0, 1))

        # Currently display_indices only looks for the first two None values
        indices = WorkspaceInfo.display_indices(slicepoint=(None, None, None), transpose=False)
        self.assertEqual(indices, (0, 1))

    def test_get_display_indices_with_transpose(self):
        indices = WorkspaceInfo.display_indices(slicepoint=(None, 0, None), transpose=True)
        self.assertEqual(indices, (2, 0))

    def test_get_display_indices_raises_error_with_incorrect_number_of_dimensions(self):
        self.assertRaises(ValueError, WorkspaceInfo.display_indices, slicepoint=(0, 0, 0), transpose=False)
        self.assertRaises(ValueError,
                          WorkspaceInfo.display_indices,
                          slicepoint=(None, 0, 0),
                          transpose=False)

    def test_can_support_dynamic_rebinning_for_MDE_workspace(self):
        with mock.patch.object(WorkspaceInfo, "get_ws_type") as mock_get_ws_type:
            mock_get_ws_type.return_value = WS_TYPE.MDE
            mock_ws = mock.NonCallableMock()

            self.assertTrue(WorkspaceInfo.can_support_dynamic_rebinning(mock_ws))
            mock_get_ws_type.assert_called_once_with(mock_ws)

    def test_can_support_dynamic_rebinning_for_MDH_workspace(self):
        with mock.patch.object(WorkspaceInfo, "get_ws_type") as mock_get_ws_type:
            mock_get_ws_type.return_value = WS_TYPE.MDH
            mock_ws = mock.NonCallableMock()
            mock_ws.hasOriginalWorkspace.return_value = True
            mock_ws.getOriginalWorkspace.return_value.getNumDims.return_value = mock_ws.getNumDims.return_value

            self.assertTrue(WorkspaceInfo.can_support_dynamic_rebinning(mock_ws))
            mock_get_ws_type.assert_called_once_with(mock_ws)
            mock_ws.hasOriginalWorkspace.assert_called_once_with(0)
            mock_ws.getOriginalWorkspace.assert_called_once_with(0)

    def test_cannot_support_dynamic_rebinning_for_MDH_workspace_without_original_workspace(self):
        with mock.patch.object(WorkspaceInfo, "get_ws_type") as mock_get_ws_type:
            mock_get_ws_type.return_value = WS_TYPE.MDH
            mock_ws = mock.NonCallableMock()
            mock_ws.hasOriginalWorkspace.return_value = False
            mock_ws.getOriginalWorkspace.return_value.getNumDims.return_value = mock_ws.getNumDims.return_value

            self.assertFalse(WorkspaceInfo.can_support_dynamic_rebinning(mock_ws))
            mock_get_ws_type.assert_called_once_with(mock_ws)

    def test_cannot_support_dynamic_rebinning_for_MDH_workspace_with_different_dims(self):
        with mock.patch.object(WorkspaceInfo, "get_ws_type") as mock_get_ws_type:
            mock_get_ws_type.return_value = WS_TYPE.MDH
            mock_ws = mock.NonCallableMock()
            mock_ws.hasOriginalWorkspace.return_value = True

            self.assertFalse(WorkspaceInfo.can_support_dynamic_rebinning(mock_ws))
            mock_get_ws_type.assert_called_once_with(mock_ws)

    def test_cannot_support_dynamic_rebinning_with_non_MD_workspace_types(self):
        for in_type in [WS_TYPE.MATRIX, None]:
            with mock.patch.object(WorkspaceInfo, "get_ws_type") as mock_get_ws_type:
                mock_get_ws_type.return_value = in_type
                mock_ws = mock.NonCallableMock()

                self.assertFalse(WorkspaceInfo.can_support_dynamic_rebinning(mock_ws))
                mock_get_ws_type.assert_called_once_with(mock_ws)


if __name__ == '__name':
    unittest.main()
