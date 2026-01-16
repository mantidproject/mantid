# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.NotebookPresenter import NotebookPresenter

import unittest
from unittest.mock import MagicMock, patch
import numpy as np


class TestNotebookPresenter(unittest.TestCase):
    def setUp(self):
        self.mock_view = MagicMock()
        self.mock_model = MagicMock()
        self.mock_model.detector_positions = np.array([[0, 0, 0], [1, 1, 1]])
        self.mock_model.detector_counts = np.array([10, 20])
        self.mock_model.picked_visibility = np.array([True, False])
        self.mock_model.is_2d_projection = True
        self.mock_model.detector_ids = np.array([101, 102])
        self.mock_model.workspace_x_unit = "Wavelength"
        self.mock_model.line_plot_workspace = MagicMock()
        with patch("instrumentview.NotebookPresenter.pv"):
            self._presenter = NotebookPresenter(self.mock_view, self.mock_model)

    def test_init_calls_model_setup_and_setup(self):
        self.mock_model.setup.assert_called_once()
        self.mock_view.subscribe_presenter.assert_called_once_with(self._presenter)

    def test_setup_creates_meshes_and_calls_view_methods(self):
        detector_mesh = self._presenter._detector_mesh
        detector_mesh.__setitem__.assert_any_call("Integrated Counts", self.mock_model.detector_counts)
        pickable_mesh = self._presenter._pickable_mesh
        pickable_mesh.__setitem__.assert_any_call("Visible Picked", self.mock_model.picked_visibility)
        self.mock_view.show_axes.assert_called_once()
        self.mock_view.add_detector_mesh.assert_called_once_with(
            detector_mesh, is_projection=self.mock_model.is_2d_projection, scalars="Integrated Counts"
        )
        self.mock_view.add_selection_mesh.assert_called_once_with(pickable_mesh, scalars="Visible Picked")
        self.mock_view.reset_camera.assert_called_once()

    def test_pick_detectors_with_valid_ids_updates_visibility_and_calls_plot(self):
        detector_ids = [101]
        indices = (np.array([0]), 0)
        with patch("instrumentview.NotebookPresenter.np.where", return_value=indices):
            result = self._presenter.pick_detectors(detector_ids, sum_spectra=True)

        expected_mask = np.array([True, False])
        self.mock_model.negate_picked_visibility.assert_called_once()
        np.testing.assert_array_equal(self.mock_model.negate_picked_visibility.call_args[0][0], expected_mask)

        self._presenter._pickable_mesh.__setitem__.assert_called_with("Visible Picked", self.mock_model.picked_visibility)
        self.mock_model.extract_spectra_for_line_plot.assert_called_once_with(self.mock_model.workspace_x_unit, True)

        self.mock_view._plot_spectra.assert_called_once_with(self.mock_model.line_plot_workspace, True)
        self.assertEqual(result, self.mock_view._plot_spectra.return_value)

    def test_pick_detectors_with_invalid_ids_prints_message_and_returns_none(self):
        with patch("instrumentview.NotebookPresenter.np.where", return_value=(np.array([]), 0)):
            result = self._presenter.pick_detectors([999], sum_spectra=False)

        self.mock_model.negate_picked_visibility.assert_not_called()
        self.mock_view._plot_spectra.assert_not_called()
        self.assertIsNone(result)


if __name__ == "__main__":
    unittest.main()
