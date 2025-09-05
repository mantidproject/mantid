# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, Mock, call
from collections import namedtuple

from mantidqt.widgets.sliceviewer.models.masking import MaskingModel


class SliceViewerMaskingModelTest(unittest.TestCase):
    TableRow = namedtuple("TableRow", ["spec_list", "x_min", "x_max"])

    def setUp(self):
        self.model = MaskingModel("test_ws_name")

    @staticmethod
    def _set_up_model_test(text, transpose=False, images=None):
        pass

    def test_clear_active_mask(self):
        self.model._active_mask = "test"
        self.model.clear_active_mask()
        self.assertIsNone(self.model._active_mask)

    def test_store_active_mask(self):
        self.model._active_mask = "test"
        self.model.store_active_mask()
        self._masks = ["test"]

    def test_store_active_mask_no_active_mask(self):
        self.model.store_active_mask()
        self.model._masks = None

    def test_clear_stored_masks(self):
        self.model._active_mask = "test"
        self.model.store_active_mask()
        self.assertEqual(self.model._masks, ["test"])
        self.model.clear_stored_masks()
        self.assertEqual([], self.model._masks)

    @patch("mantidqt.widgets.sliceviewer.models.masking.RectCursorInfo")
    def test_add_rect_cursor_info(self, CursorInfo_mock):
        kwargs = {"click": "test_click", "release": "test_release", "transpose": "test_transpose"}
        self.model.add_rect_cursor_info(**kwargs)
        CursorInfo_mock.assert_called_once_with(**kwargs)

    @patch("mantidqt.widgets.sliceviewer.models.masking.ElliCursorInfo")
    def test_add_elli_cursor_info(self, CursorInfo_mock):
        kwargs = {"click": "test_click", "release": "test_release", "transpose": "test_transpose"}
        self.model.add_elli_cursor_info(**kwargs)
        CursorInfo_mock.assert_called_once_with(**kwargs)

    @patch("mantidqt.widgets.sliceviewer.models.masking.PolyCursorInfo")
    def test_add_poly_cursor_info(self, CursorInfo_mock):
        kwargs = {"nodes": "nodes", "transpose": "test_transpose"}
        self.model.add_poly_cursor_info(**kwargs)
        CursorInfo_mock.assert_called_once_with(**kwargs)

    def test_create_table_workspace_from_rows(self):
        with patch("mantidqt.widgets.sliceviewer.models.masking.AnalysisDataService") as ads_mock:
            table_ws = self.model.create_table_workspace_from_rows(
                [self.TableRow("1", 5, 8), self.TableRow("2", 6, 9), self.TableRow("3-10", 7, 10)], False
            )
            self.assertEqual(table_ws.column("SpectraList"), ["1", "2", "3-10"])
            self.assertEqual(table_ws.column("XMin"), [5, 6, 7])
            self.assertEqual(table_ws.column("XMax"), [8, 9, 10])
            ads_mock.addOrReplace.assert_not_called()

    def test_create_table_workspace_from_rows_store_in_ads(self):
        with patch("mantidqt.widgets.sliceviewer.models.masking.AnalysisDataService") as ads_mock:
            table_ws = self.model.create_table_workspace_from_rows(
                [self.TableRow("1", 5, 8), self.TableRow("2", 6, 9), self.TableRow("3-10", 7, 10)], True
            )
            self.assertEqual(table_ws.column("SpectraList"), ["1", "2", "3-10"])
            self.assertEqual(table_ws.column("XMin"), [5, 6, 7])
            self.assertEqual(table_ws.column("XMax"), [8, 9, 10])
            ads_mock.addOrReplace.assert_called_once_with("test_ws_name_sv_mask_tbl", table_ws)

    def test_generate_mask_table_ws(self):
        mock_masks = [Mock(generate_table_rows=Mock(return_value=[f"test_{i}"])) for i in range(3)]
        self.model._masks = mock_masks
        with patch("mantidqt.widgets.sliceviewer.models.masking.MaskingModel.create_table_workspace_from_rows") as create_tbl_mock:
            self.model.generate_mask_table_ws(store_in_ads=False)
            create_tbl_mock.assert_called_once_with(["test_0", "test_1", "test_2"], False)

    def test_export_selectors(self):
        with (
            patch("mantidqt.widgets.sliceviewer.models.masking.AnalysisDataService") as ads_mock,
            patch("mantidqt.widgets.sliceviewer.models.masking.MaskingModel.generate_mask_table_ws") as gen_mask_tbl_mock,
        ):
            self.model.export_selectors()
            ads_mock.assert_not_called()
            gen_mask_tbl_mock.assert_called_once_with(store_in_ads=True)

    def test_apply_selectors(self):
        with (
            patch("mantidqt.widgets.sliceviewer.models.masking.AnalysisDataService") as ads_mock,
            patch(
                "mantidqt.widgets.sliceviewer.models.masking.MaskingModel.generate_mask_table_ws", return_value="test_ws"
            ) as gen_mask_tbl_mock,
            patch("mantidqt.widgets.sliceviewer.models.masking.AlgorithmManager") as alg_manager_mock,
        ):
            alg_mock = alg_manager_mock.create.return_value
            self.model.apply_selectors()
            ads_mock.assert_not_called()
            gen_mask_tbl_mock.assert_called_once_with(store_in_ads=False)
            alg_mock.create.called_once_with("MaksBinsFromTable")
            alg_mock.setProperty.assert_has_calls(
                [
                    call("InputWorkspace", "test_ws_name"),
                    call("OutputWorkspace", "test_ws_name"),
                    call("MaskingInformation", "test_ws"),
                    call("InputWorkspaceIndexType", "SpectrumNumber"),
                ]
            )
            alg_mock.execute.assert_called_once()


class CursorInfoTest(unittest.TestCase):
    Click = namedtuple("Click", ["xdata", "ydata"])

    @staticmethod
    def _set_up_model_test(text, transpose=False, images=None):
        pass

    def test_clear_active_mask(self):
        pass

    def test_store_active_mask(self):
        pass


if __name__ == "__main__":
    unittest.main()
