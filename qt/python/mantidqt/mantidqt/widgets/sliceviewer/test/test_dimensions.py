# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import MatrixWorkspace
from mantidqt.widgets.sliceviewer.models.dimensions import Dimensions


class TestDimensions(unittest.TestCase):
    def setUp(self) -> None:
        self._mock_ws = mock.Mock(spec=MatrixWorkspace)

    @mock.patch("mantidqt.widgets.sliceviewer.models.dimensions.WorkspaceInfo")
    def test_get_dim_limits_returns_limits_for_display_dimensions_for_matrix(self, mock_ws_info):
        xindex, yindex = mock.NonCallableMock(), mock.NonCallableMock()
        mock_ws_info.display_indices.return_value = xindex, yindex
        xdim, ydim = mock.Mock(), mock.Mock(),
        self._mock_ws.getDimension.side_effect = [xdim, ydim]

        limits = Dimensions.get_dim_limits(self._mock_ws, slicepoint=(None, None), transpose=False)
        self._mock_ws.getDimension.assert_any_call(xindex)
        self._mock_ws.getDimension.assert_any_call(yindex)
        self.assertEqual(2, self._mock_ws.getDimension.call_count)

        self.assertEqual(limits[0], (xdim.getMinimum.return_value, xdim.getMaximum.return_value))
        self.assertEqual(limits[1], (ydim.getMinimum.return_value, ydim.getMaximum.return_value))

    @mock.patch("mantidqt.widgets.sliceviewer.models.dimensions.WorkspaceInfo")
    def test_get_dim_info(self, mock_ws_info):
        num = mock.NonCallableMock()

        result = Dimensions.get_dim_info(self._mock_ws, num)
        dim = self._mock_ws.getDimension.return_value

        self._mock_ws.getDimension.assert_called_once_with(num)
        mock_ws_info.get_ws_type.assert_called_once_with(self._mock_ws)
        mock_ws_info.can_support_dynamic_rebinning.assert_called_once_with(self._mock_ws)

        self.assertEqual(dim.getMinimum(), result['minimum'])
        self.assertEqual(dim.getMaximum(), result['maximum'])
        self.assertEqual(dim.getNBins(), result['number_of_bins'])
        self.assertEqual(dim.getBinWidth(), result['width'])
        self.assertEqual(dim.getUnits(), result['units'])
        self.assertEqual(dim.name, result['name'])
        self.assertEqual(mock_ws_info.get_ws_type().name, result['type'])
        self.assertEqual(mock_ws_info.can_support_dynamic_rebinning(), result['can_rebin'])
        self.assertEqual(dim.getMDFrame().isQ(), result['qdim'])

    def test_get_dimensions_info(self):
        with mock.patch("mantidqt.widgets.sliceviewer.test.test_dimensions.Dimensions.get_dim_info") \
                as mock_get_dim_info:
            expected_num = 3
            self._mock_ws.getNumDims.return_value = expected_num
            expected_results = [mock.NonCallableMock() for _ in range(expected_num)]
            mock_get_dim_info.side_effect = expected_results

            result = Dimensions.get_dimensions_info(self._mock_ws)

            calls = [mock.call(self._mock_ws, i) for i in range(expected_num)]
            mock_get_dim_info.assert_has_calls(calls)
            self.assertEqual(expected_results, result)


if __name__ == '__main__':
    unittest.main()
