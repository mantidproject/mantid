# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock

import numpy as np

from instrumentview.alfview.ALFInstrumentViewPresenter import ALFInstrumentViewPresenter
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.Globals import CurrentTab
from instrumentview.Projections.ProjectionType import ProjectionType


class TestALFInstrumentViewPresenter(unittest.TestCase):
    def setUp(self):
        self._mock_view = MagicMock()
        self._mock_view.current_selected_projection.return_value = ProjectionType.CYLINDRICAL_Y
        self._mock_view.get_render_mode_option.return_value = "Points"
        self._mock_view._RENDER_MODE_POINTS = "Points (Fastest)"
        self._mock_view._RENDER_MODE_SHAPES_FAST = "Approximated Shapes (Fast)"
        self._mock_view._RENDER_MODE_RAW_SHAPES = "Raw Shapes (Slowest)"
        self._mock_view.is_hover_pick_mode_checked.return_value = False
        self._mock_view.u_offset_degrees.return_value = 0
        self._mock_view.is_rotate_180_checkbox_checked.return_value = False
        self._mock_view.is_select_bank_tube_checked.return_value = True
        self._mock_view.get_contour_limits.return_value = (0.0, 1.0)
        self._mock_view.selected_peaks_workspaces.return_value = []
        self._mock_view.findChild.return_value = None

        with (
            mock.patch("instrumentview.alfview.ALFInstrumentViewPresenter.ALFInstrumentViewView", return_value=self._mock_view),
            mock.patch("instrumentview.FullInstrumentViewPresenter.InstrumentViewADSObserver"),
        ):
            self._presenter = ALFInstrumentViewPresenter()
        self._model = self._presenter._model
        self._mock_view.reset_mock()
        self._n_pickable = len(self._model.detector_positions)

    def tearDown(self):
        self._presenter.handle_close()

    def _mask_for_first_n_pickable_detectors(self, n: int) -> np.ndarray:
        mask = np.zeros(self._n_pickable, dtype=bool)
        mask[:n] = True
        return mask

    def test_on_roi_shape_changed_projects_points_before_queueing_the_update(self):
        self._presenter._callback_queue = MagicMock()

        self._presenter.on_roi_shape_changed()

        self._mock_view.project_and_cache_detector_points.assert_called_once()
        queued_function, queued_args = self._presenter._callback_queue.put.call_args.args[0]
        self.assertEqual(queued_function, self._presenter._on_roi_shape_changed)
        np.testing.assert_allclose(queued_args[0], self._mock_view.project_and_cache_detector_points.call_args.args[0])

    def test_on_roi_shape_changed_picks_the_detectors_inside_the_shape(self):
        mask = self._mask_for_first_n_pickable_detectors(5)
        self._mock_view.get_shape_mask.return_value = mask
        self._mock_view.is_select_bank_tube_checked.return_value = False

        with mock.patch.object(self._presenter, "_publish_selection_change"):
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))

        np.testing.assert_array_equal(self._model._detector_is_picked[self._model.is_pickable], mask)

    def test_on_roi_shape_changed_expands_the_selection_to_whole_tubes(self):
        expanded_mask = self._mask_for_first_n_pickable_detectors(64)
        self._mock_view.get_shape_mask.return_value = self._mask_for_first_n_pickable_detectors(5)
        self._mock_view.is_select_bank_tube_checked.return_value = True
        self._model.expand_pickable_mask_to_parent_subtrees = MagicMock(return_value=expanded_mask)

        with mock.patch.object(self._presenter, "_publish_selection_change"):
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))

        self._model.expand_pickable_mask_to_parent_subtrees.assert_called_once()
        np.testing.assert_array_equal(self._model._detector_is_picked[self._model.is_pickable], expanded_mask)

    def test_moving_the_shape_replaces_the_previous_selection(self):
        first_mask = self._mask_for_first_n_pickable_detectors(10)
        second_mask = np.zeros(self._n_pickable, dtype=bool)
        second_mask[20:30] = True
        self._mock_view.is_select_bank_tube_checked.return_value = False

        with mock.patch.object(self._presenter, "_publish_selection_change"):
            self._mock_view.get_shape_mask.return_value = first_mask
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))
            self._mock_view.get_shape_mask.return_value = second_mask
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))

        np.testing.assert_array_equal(self._model._detector_is_picked[self._model.is_pickable], second_mask)
        self.assertEqual(self._model.cached_pick_selections_keys, [ALFInstrumentViewPresenter._ROI_SELECTION_KEY])

    def test_empty_shape_clears_the_selection(self):
        self._mock_view.is_select_bank_tube_checked.return_value = False

        with mock.patch.object(self._presenter, "_publish_selection_change"):
            self._mock_view.get_shape_mask.return_value = self._mask_for_first_n_pickable_detectors(10)
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))
            self._mock_view.get_shape_mask.return_value = np.zeros(self._n_pickable, dtype=bool)
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))

        self.assertFalse(np.any(self._model._detector_is_picked))

    def test_on_roi_shape_changed_stores_the_selection_as_a_grouping_item(self):
        mask = self._mask_for_first_n_pickable_detectors(5)
        self._mock_view.get_shape_mask.return_value = mask
        self._mock_view.is_select_bank_tube_checked.return_value = False
        self._model.set_detector_key = MagicMock(return_value=ALFInstrumentViewPresenter._ROI_SELECTION_KEY)

        with mock.patch.object(self._presenter, "_publish_selection_change"):
            self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))

        self._model.set_detector_key.assert_called_once_with(
            ALFInstrumentViewPresenter._ROI_SELECTION_KEY, mask.tolist(), CurrentTab.Grouping
        )

    def test_on_roi_shape_changed_updates_the_view_and_notifies_alf(self):
        self._mock_view.get_shape_mask.return_value = self._mask_for_first_n_pickable_detectors(5)
        self._mock_view.is_select_bank_tube_checked.return_value = False

        with mock.patch.object(self._presenter, "notify_cpp_callback") as mock_notify:
            with mock.patch.object(ALFInstrumentViewPresenter.__bases__[0], "update_picked_detectors_on_view"):
                self._presenter._on_roi_shape_changed(np.zeros((self._n_pickable, 3)))

        mock_notify.assert_called_once_with("notify_whole_tube_selected")

    def test_replace_workspace_callback_preserves_picked_detectors_after_reset(self):
        expected_detector_is_picked = np.zeros(self._n_pickable, dtype=bool)
        expected_point_picked_detectors = np.zeros(self._n_pickable, dtype=bool)
        expected_detector_is_picked[0:3] = True
        expected_point_picked_detectors[1:4] = True
        self._model._detector_is_picked = expected_detector_is_picked.copy()
        self._model._point_picked_detectors = expected_point_picked_detectors.copy()

        def fake_reset(_ws_name: str, _ws):
            self._model._detector_is_picked[:] = False
            self._model._point_picked_detectors[:] = False

        with (
            mock.patch.object(FullInstrumentViewPresenter, "_replace_workspace_callback", side_effect=fake_reset),
            mock.patch.object(self._presenter, "_update_view_main_plotter"),
            mock.patch.object(self._presenter, "_publish_selection_change"),
        ):
            self._presenter._replace_workspace_callback("workspace", None)

        np.testing.assert_array_equal(self._model._detector_is_picked, expected_detector_is_picked)
        np.testing.assert_array_equal(self._model._point_picked_detectors, expected_point_picked_detectors)

    def test_replace_workspace_callback_does_not_restore_cached_detectors_when_shape_changes(self):
        cached_detector_is_picked = np.array([True, False, True], dtype=bool)
        cached_point_picked_detectors = np.array([False, True, False], dtype=bool)
        self._model._detector_is_picked = cached_detector_is_picked.copy()
        self._model._point_picked_detectors = cached_point_picked_detectors.copy()

        def fake_reset(_ws_name: str, _ws):
            self._model._detector_is_picked = np.zeros(1, dtype=bool)
            self._model._point_picked_detectors = np.zeros(1, dtype=bool)

        with (
            mock.patch.object(FullInstrumentViewPresenter, "_replace_workspace_callback", side_effect=fake_reset),
            mock.patch.object(self._presenter, "_update_view_main_plotter"),
            mock.patch.object(self._presenter, "_publish_selection_change"),
        ):
            self._presenter._replace_workspace_callback("workspace", None)

        np.testing.assert_array_equal(self._model._detector_is_picked, np.zeros(1, dtype=bool))
        np.testing.assert_array_equal(self._model._point_picked_detectors, np.zeros(1, dtype=bool))

    def test_replace_workspace_callback_notifies_instrument_actor_reset(self):
        with (
            mock.patch.object(FullInstrumentViewPresenter, "_replace_workspace_callback"),
            mock.patch.object(self._presenter, "notify_cpp_callback") as mock_notify,
        ):
            self._presenter._replace_workspace_callback("workspace", None)

        self.assertEqual(
            mock_notify.call_args_list,
            [mock.call("notify_whole_tube_selected"), mock.call("notify_instrument_actor_reset")],
        )


if __name__ == "__main__":
    unittest.main()
