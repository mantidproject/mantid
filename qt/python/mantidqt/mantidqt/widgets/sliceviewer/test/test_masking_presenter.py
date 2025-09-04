# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, Mock
from mantidqt.widgets.sliceviewer.presenters.masking import Masking
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText


class SliceViewerBasePresenterTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self) -> None:
        pass

    @staticmethod
    def _setup_presenter_test(active_selector=None, selector_returns=1):
        mock_rect_selector_objs = [Mock() for _ in range(selector_returns)]
        mock_rect_selector_cls = Mock(side_effect=mock_rect_selector_objs)
        mock_dataview = Mock()
        presenter = Masking(dataview=mock_dataview, ws_name=Mock())
        presenter._active_selector = active_selector
        return presenter, {
            "selector_cls": mock_rect_selector_cls,
            "selector_obj": mock_rect_selector_objs,
            "dataview": mock_dataview,
            "active_selector": active_selector,
        }

    @patch("mantidqt.widgets.sliceviewer.presenters.masking.MaskingModel", autospec=True)
    def test_new_selector_no_active_selector(self, mock_masking_model_fn):
        mock_model = mock_masking_model_fn.return_value
        presenter, mocks = self._setup_presenter_test()
        with patch(
            "mantidqt.widgets.sliceviewer.presenters.masking.TOOL_ITEM_TEXT_TO_SELECTOR", {ToolItemText.RECT_MASKING: mocks["selector_cls"]}
        ):
            self.assertIsNone(presenter._active_selector)
            presenter.new_selector(text=ToolItemText.RECT_MASKING)
            mocks["selector_cls"].called_once_with(mocks["dataview"], mock_model)
            mocks["selector_obj"][0].set_active.assert_called_once_with(True)

    @patch("mantidqt.widgets.sliceviewer.presenters.masking.MaskingModel", autospec=True)
    def test_new_selector_active_selector_reset_no_mask_drawn(self, mock_masking_model_fn):
        mock_model = mock_masking_model_fn.return_value
        presenter, mocks = self._setup_presenter_test(active_selector=Mock(mask_drawn=False))
        with patch(
            "mantidqt.widgets.sliceviewer.presenters.masking.TOOL_ITEM_TEXT_TO_SELECTOR", {ToolItemText.RECT_MASKING: mocks["selector_cls"]}
        ):
            presenter.new_selector(text=ToolItemText.RECT_MASKING)
            mocks["selector_cls"].assert_called_once_with(mocks["dataview"], mock_model)
            mocks["active_selector"].set_active.assert_called_once_with(False)
            mocks["active_selector"].disconnect.assert_called_once()
            mocks["active_selector"].clear.assert_called_once()
            mocks["selector_obj"][0].set_active.assert_called_once_with(True)
            mock_model.clear_active_mask.assert_called_once()
            self.assertEqual(presenter._selectors, [])

    @patch("mantidqt.widgets.sliceviewer.presenters.masking.MaskingModel", autospec=True)
    def test_new_selector_active_selector_reset_mask_drawn(self, mock_masking_model_fn):
        mock_model = mock_masking_model_fn.return_value
        presenter, mocks = self._setup_presenter_test(active_selector=Mock(mask_drawn=True))
        with patch(
            "mantidqt.widgets.sliceviewer.presenters.masking.TOOL_ITEM_TEXT_TO_SELECTOR", {ToolItemText.RECT_MASKING: mocks["selector_cls"]}
        ):
            presenter.new_selector(text=ToolItemText.RECT_MASKING)
            mocks["selector_cls"].assert_called_once_with(mocks["dataview"], mock_model)
            mocks["active_selector"].set_active.assert_called_once_with(False)
            mocks["active_selector"].disconnect.assert_called_once()
            mocks["active_selector"].set_inactive_color.assert_called_once()
            mock_model.store_active_mask.assert_called_once()
            self.assertEqual([mocks["active_selector"]], presenter._selectors)

    @patch("mantidqt.widgets.sliceviewer.presenters.masking.MaskingModel", autospec=True)
    def test_export_selectors(self, mock_masking_model_fn):
        mock_model = mock_masking_model_fn.return_value
        presenter, mocks = self._setup_presenter_test()
        with patch.object(Masking, "_reset_active_selector", autospec=True, wraps=Masking._reset_active_selector) as reset_spy:
            presenter.export_selectors()
            reset_spy.assert_called_once()
            mock_model.export_selectors.assert_called_once()

    @patch("mantidqt.widgets.sliceviewer.presenters.masking.MaskingModel", autospec=True)
    def test_apply_selectors(self, mock_masking_model_fn):
        mock_model = mock_masking_model_fn.return_value
        presenter, mocks = self._setup_presenter_test(active_selector=Mock(mask_drawn=True))
        with patch.object(Masking, "_reset_active_selector", autospec=True, wraps=Masking._reset_active_selector) as reset_spy:
            presenter.apply_selectors()
            reset_spy.assert_called_once()
            mock_model.apply_selectors.assert_called_once()

    def test_clear_and_disconnect_no_active_selector(self):
        presenter, mocks = self._setup_presenter_test()
        mock_selectors = [Mock() for _ in range(3)]
        presenter._selectors = mock_selectors
        self.assertIsNone(presenter._active_selector)
        presenter.clear_and_disconnect()
        for mock_selector in mock_selectors:
            mock_selector.clear.assert_called_once()

    def test_clear_and_disconnect_active_selector_with_mask(self):
        presenter, mocks = self._setup_presenter_test(active_selector=Mock(mask_drawn=True))
        mock_selectors = [Mock() for _ in range(3)]
        presenter._selectors = mock_selectors
        presenter.clear_and_disconnect()
        mocks["active_selector"].set_active.assert_called_once_with(False)
        mocks["active_selector"].disconnect.assert_called_once()
        mocks["active_selector"].clear.assert_called_once()
        for mock_selector in mock_selectors:
            mock_selector.clear.assert_called_once()

    @patch("mantidqt.widgets.sliceviewer.presenters.masking.MaskingModel", autospec=True)
    def test_clear_model(self, mock_masking_model_fn):
        mock_model = mock_masking_model_fn.return_value
        presenter, mocks = self._setup_presenter_test()
        presenter.clear_model()
        mock_model.clear_active_mask.assert_called_once()
        mock_model.clear_stored_masks.assert_called_once()


if __name__ == "__main__":
    unittest.main()
