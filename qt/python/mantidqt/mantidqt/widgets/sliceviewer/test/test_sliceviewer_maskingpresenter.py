# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, Mock, call
from collections import namedtuple

from mantidqt.widgets.sliceviewer.presenters.masking import Masking, INACTIVE_HANDLE_STYLE, INACTIVE_SHAPE_STYLE, TOOL_ITEM_TEXT_TO_SELECTOR
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText

from matplotlib.widgets import EllipseSelector


class SliceViewerMaskingPresenterTest(unittest.TestCase):
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


class SliceViewerSelectionMaskingTest(unittest.TestCase):
    Click = namedtuple("Click", ["xdata", "ydata"])

    @staticmethod
    def _set_up_selector_test(text, transpose=False, images=None):
        mock_dataview = Mock()
        mock_dataview.dimensions.get_states.return_value = [1, 0] if transpose else [0, 1]
        mock_dataview.ax.images = images
        mock_model = Mock()
        selector = TOOL_ITEM_TEXT_TO_SELECTOR[text](mock_dataview, mock_model)
        return selector, {
            "dataview": mock_dataview,
            "model": mock_model,
        }

    def test_add_cursor_info_rectangle(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class"):
            selector, mocks = self._set_up_selector_test(ToolItemText.RECT_MASKING)

            selector.add_cursor_info("test_click", "test_release")
            mocks["model"].add_rect_cursor_info.assert_called_once_with(click="test_click", release="test_release", transpose=False)

    def test_add_cursor_info_elli(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class"):
            selector, mocks = self._set_up_selector_test(ToolItemText.ELLI_MASKING)

            selector.add_cursor_info("test_click", "test_release")
            mocks["model"].add_elli_cursor_info.assert_called_once_with(click="test_click", release="test_release", transpose=False)

    def _test_inactive_color_rect_base(self, text):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, _ = self._set_up_selector_test(text)

            selector.set_inactive_color()
            internal_selector_mock.set_props.assert_called_once_with(fill=False, **INACTIVE_SHAPE_STYLE)
            internal_selector_mock.set_handle_props.assert_called_once_with(**INACTIVE_HANDLE_STYLE)

    def test_set_inactive_color_rectangle(self):
        self._test_inactive_color_rect_base(ToolItemText.RECT_MASKING)

    def test_set_inactive_color_elli(self):
        self._test_inactive_color_rect_base(ToolItemText.ELLI_MASKING)

    def test_set_inactive_color_poly(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, _ = self._set_up_selector_test(ToolItemText.POLY_MASKING)

            selector.set_inactive_color()
            internal_selector_mock.set_props.assert_called_once_with(**INACTIVE_SHAPE_STYLE)
            internal_selector_mock.set_handle_props.assert_called_once_with(**INACTIVE_HANDLE_STYLE)

    def test_set_active(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, _ = self._set_up_selector_test(ToolItemText.POLY_MASKING)

            selector.set_active(False)
            selector.set_active(True)
            internal_selector_mock.set_active.assert_has_calls([call(False), call(True)])

    def test_clear(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            mock_artists = [Mock() for _ in range(3)]
            internal_selector_mock.artists = mock_artists
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, _ = self._set_up_selector_test(ToolItemText.ELLI_MASKING)

            selector.clear()
            selector.clear()  # test second clear doesn't error
            for artist in mock_artists:
                artist.remove.assert_called_once()

    def test_disconnect(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, _ = self._set_up_selector_test(ToolItemText.RECT_MASKING)

            selector.disconnect()
            internal_selector_mock.disconnect_events.assert_called_once()

    def test_remove_mask_from_model(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, mocks = self._set_up_selector_test(ToolItemText.RECT_MASKING)

            selector.remove_mask_from_model()
            mocks["model"].clear_active_mask.assert_not_called()
            selector._mask_drawn = True
            selector.remove_mask_from_model()
            mocks["model"].clear_active_mask.assert_called_once()

    def test_init(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock

            selector, mocks = self._set_up_selector_test(ToolItemText.ELLI_MASKING)
            self.assertFalse(selector._transpose)
            selector_fn_mock.assert_called_with(EllipseSelector, selector.remove_mask_from_model)
            _, args, kwargs = selector_fn_mock.return_value.mock_calls[0]
            self.assertEqual(args[1], selector._on_selected)

    def test_init_transpose(self):
        with patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock:
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock

            selector, mocks = self._set_up_selector_test(ToolItemText.ELLI_MASKING, transpose=True)
            self.assertTrue(selector._transpose)

    def test_on_selected_rect(self):
        with (
            patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock,
            patch("mantidqt.widgets.sliceviewer.presenters.masking.cursor_info") as cursor_info_fn,
            patch("mantidqt.widgets.sliceviewer.presenters.masking.RectangleSelectionMasking.add_cursor_info") as add_cursor_info_mock,
        ):
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, mocks = self._set_up_selector_test(ToolItemText.RECT_MASKING, images=["test"])
            cursor_info_mock = [Mock() for _ in range(2)]
            cursor_info_fn.side_effect = cursor_info_mock

            selector._on_selected(self.Click(0, 0), self.Click(10, 10))
            mocks["dataview"].mpl_toolbar.set_action_checked.assert_called_once_with(ToolItemText.RECT_MASKING, False, trigger=False)
            add_cursor_info_mock.assert_called_once_with(*cursor_info_mock)

    def test_on_selected_poly(self):
        with (
            patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock,
            patch("mantidqt.widgets.sliceviewer.presenters.masking.cursor_info") as cursor_info_fn,
        ):
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, mocks = self._set_up_selector_test(ToolItemText.POLY_MASKING, images=["test"])
            cursor_info_mock = [Mock() for _ in range(3)]
            cursor_info_fn.side_effect = cursor_info_mock

            selector._on_selected([self.Click(0, 0), self.Click(5, 5), self.Click(0, 10)])
            mocks["dataview"].mpl_toolbar.set_action_checked.assert_called_once_with(ToolItemText.POLY_MASKING, False, trigger=False)
            mocks["model"].add_poly_cursor_info.assert_called_with(nodes=cursor_info_mock, transpose=False)

    def test_on_selected_poly_error(self):
        with (
            patch("mantidqt.widgets.sliceviewer.presenters.masking.make_selector_class") as selector_fn_mock,
            patch("mantidqt.widgets.sliceviewer.presenters.masking.cursor_info") as cursor_info_fn,
        ):
            internal_selector_mock = Mock()
            selector_fn_mock.return_value.return_value = internal_selector_mock
            selector, mocks = self._set_up_selector_test(ToolItemText.POLY_MASKING, images=["test"])
            selector.clear = Mock()
            selector.disconnect = Mock()
            selector.set_active = Mock()
            cursor_info_mock = [Mock() for _ in range(3)]
            cursor_info_fn.side_effect = cursor_info_mock
            mocks["model"].add_poly_cursor_info.side_effect = RuntimeError("test")

            selector._on_selected([self.Click(0, 0), self.Click(5, 5), self.Click(0, 10)])
            mocks["model"].add_poly_cursor_info.assert_called_with(nodes=cursor_info_mock, transpose=False)
            selector.clear.assert_called_once()
            selector.disconnect.assert_called_once()
            selector.set_active.assert_called_once_with(False)


if __name__ == "__main__":
    unittest.main()
