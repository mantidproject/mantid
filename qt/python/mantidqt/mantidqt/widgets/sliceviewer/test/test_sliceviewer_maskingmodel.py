# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, Mock, call
from collections import namedtuple
from functools import partial

from mantidqt.widgets.sliceviewer.models.masking import MaskingModel, RectCursorInfo, TableRow, ElliCursorInfo, PolyCursorInfo
from numpy import float64


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
    Click = namedtuple("Click", ["data"])

    @staticmethod
    def _set_up_model_test(text, transpose=False, images=None):
        pass

    def _assert_table_rows_delta(self, actual, expected, delta):
        for i in range(len(expected)):
            expected_row = expected[i]
            for j in range(len(expected_row.__dict__)):
                expected_val = list(expected_row.__dict__.values())[j]
                actual_val = list(actual[i].__dict__.values())[j]
                assert_fn = (
                    self.assertEqual
                    if (isinstance(actual_val, str) or isinstance(expected_val, str))
                    else partial(self.assertAlmostEqual, delta=delta)
                )
                assert_fn(expected_val, actual_val, msg=f"Error in Row index {i}")

    def test_rect_cursor_info_generate_table_rows(self):
        cursor_info = RectCursorInfo(self.Click((-5.2, 0.8)), self.Click((5.6, 10.8)), False)
        row = cursor_info.generate_table_rows()
        self.assertEqual(row, [TableRow(spec_list="1-11", x_min=-5.2, x_max=5.6)])

    def test_elli_cursor_info_generate_table_rows(self):
        cursor_info = ElliCursorInfo(self.Click((-5, 0)), self.Click((5, 10)), False)
        actual_rows = cursor_info.generate_table_rows()
        expected_table_rows = [
            TableRow(spec_list="0", x_min=-1.795, x_max=1.795),
            TableRow(spec_list="1", x_min=-3.399, x_max=3.399),
            TableRow(spec_list="2", x_min=-4.229, x_max=4.229),
            TableRow(spec_list="3", x_min=-4.714, x_max=4.714),
            TableRow(spec_list="4", x_min=-4.955, x_max=4.955),
            TableRow(spec_list="5", x_min=-5.0, x_max=5.0),
            TableRow(spec_list="6", x_min=-4.955, x_max=4.955),
            TableRow(spec_list="7", x_min=-4.714, x_max=4.714),
            TableRow(spec_list="8", x_min=-4.229, x_max=4.229),
            TableRow(spec_list="9", x_min=-3.399, x_max=3.399),
            TableRow(spec_list="10", x_min=-1.795, x_max=1.795),
        ]
        self._assert_table_rows_delta(expected_table_rows, actual_rows, delta=0.001)

    def test_poly_cursor_info_generate_table_rows(self):
        # Actual alg uses float64. This is important as divide by 0 returns inf rather than an exception.
        cursor_info = PolyCursorInfo(
            [self.Click((float64(0), float64(0))), self.Click((float64(10), float64(5))), self.Click((float64(0), float64(10)))], False
        )
        actual_rows = cursor_info.generate_table_rows()
        expected_table_rows = [
            TableRow(spec_list="0", x_min=0, x_max=0.666),
            TableRow(spec_list="1", x_min=0, x_max=2.666),
            TableRow(spec_list="2", x_min=0, x_max=4.666),
            TableRow(spec_list="3", x_min=0, x_max=6.666),
            TableRow(spec_list="4", x_min=0, x_max=8.666),
            TableRow(spec_list="5", x_min=0, x_max=10.0),
            TableRow(spec_list="6", x_min=0, x_max=8.666),
            TableRow(spec_list="7", x_min=0, x_max=6.666),
            TableRow(spec_list="8", x_min=0, x_max=4.666),
            TableRow(spec_list="9", x_min=0, x_max=2.666),
            TableRow(spec_list="10", x_min=0, x_max=0.666),
        ]
        self._assert_table_rows_delta(expected_table_rows, actual_rows, delta=0.001)

    def test_poly_cursor_info_intersecting_line_once(self):
        cursor_info = PolyCursorInfo(
            [
                self.Click((float64(0), float64(0))),
                self.Click((float64(10), float64(10))),
                self.Click((float64(0), float64(10))),
                self.Click((float64(10), float64(0))),
            ],
            False,
        )
        actual_rows = cursor_info.generate_table_rows()
        expected_table_rows = [
            TableRow(spec_list="0", x_min=0.333, x_max=9.666),
            TableRow(spec_list="1", x_min=0.666, x_max=9.333),
            TableRow(spec_list="2", x_min=1.666, x_max=8.333),
            TableRow(spec_list="3", x_min=2.666, x_max=7.333),
            TableRow(spec_list="4", x_min=3.666, x_max=6.333),
            TableRow(spec_list="5", x_min=4.666, x_max=5.333),
            TableRow(spec_list="6", x_min=3.666, x_max=6.333),
            TableRow(spec_list="7", x_min=2.666, x_max=7.333),
            TableRow(spec_list="8", x_min=1.666, x_max=8.333),
            TableRow(spec_list="9", x_min=0.666, x_max=9.333),
            TableRow(spec_list="10", x_min=0.0, x_max=10.0),
        ]
        self._assert_table_rows_delta(expected_table_rows, actual_rows, delta=0.001)

    def test_poly_cursor_info_intersecting_line_twice(self):
        with self.assertRaisesRegex(
            expected_exception=RuntimeError, expected_regex="Polygon shapes with more than 1 intersection point are not supported."
        ):
            _ = PolyCursorInfo(
                [
                    self.Click((float64(0), float64(0))),
                    self.Click((float64(10), float64(10))),
                    self.Click((float64(0), float64(10))),
                    self.Click((float64(10), float64(0))),
                    self.Click((float64(10), float64(5))),
                ],
                False,
            )


if __name__ == "__main__":
    unittest.main()
