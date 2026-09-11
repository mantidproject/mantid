# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.NotebookPresenter import NotebookPresenter

from mantid.simpleapi import CreateSampleWorkspace

import unittest
from unittest.mock import MagicMock, patch
import numpy as np


class TestNotebookPresenter(unittest.TestCase):
    def setUp(self):
        self.mock_view = MagicMock()
        # Specced against the real model so that a call to an attribute the model does not have fails the test
        self.mock_model = MagicMock(spec=FullInstrumentViewModel)
        self.mock_model.detector_positions = np.array([[0, 0, 0], [1, 1, 1]])
        self.mock_model.detector_counts = np.array([10, 20])
        self.mock_model.picked_visibility = np.array([True, False])
        self.mock_model.is_2d_projection = True
        self.mock_model.pickable_detector_ids = np.array([101, 102])
        self.mock_model.workspace_base_unit = "Wavelength"
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
        result = self._presenter.pick_detectors([101], sum_spectra=True)

        # 101 is the first of the pickable detector IDs, so it is index 0
        self.mock_model.update_point_picked_detectors.assert_called_once_with(0, False, False)

        self._presenter._pickable_mesh.__setitem__.assert_called_with("Visible Picked", self.mock_model.picked_visibility)
        self.mock_model.extract_spectra_for_line_plot.assert_called_once_with("Wavelength", True)

        self.mock_view.plot_spectra.assert_called_once_with(self.mock_model.line_plot_workspace, True)
        self.assertEqual(result, self.mock_view.plot_spectra.return_value)

    def test_pick_detectors_with_several_valid_ids_updates_each_detector(self):
        self._presenter.pick_detectors([101, 102], sum_spectra=False)

        self.assertEqual(
            [call.args for call in self.mock_model.update_point_picked_detectors.call_args_list],
            [(0, False, False), (1, False, False)],
        )

    def test_pick_detectors_with_invalid_ids_prints_message_and_returns_none(self):
        with self.assertWarns(UserWarning):
            result = self._presenter.pick_detectors([999], sum_spectra=False)

        self.mock_model.update_point_picked_detectors.assert_not_called()
        self.mock_view.plot_spectra.assert_not_called()
        self.assertIsNone(result)

    def test_pick_detectors_returns_none_when_no_spectra_extracted(self):
        self.mock_model.line_plot_workspace = None

        result = self._presenter.pick_detectors([101], sum_spectra=True)

        self.mock_view.plot_spectra.assert_not_called()
        self.assertIsNone(result)


class TestNotebookPresenterWithRealModel(unittest.TestCase):
    """Exercises the presenter against a real model, which is what caught the model API drift that the mocked tests missed."""

    def setUp(self):
        self._workspace = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, StoreInADS=False)
        self._model = FullInstrumentViewModel(self._workspace)
        self._view = MagicMock()
        self._presenter = NotebookPresenter(self._view, self._model)

    def test_pick_detectors_picks_the_requested_detectors_and_plots_them(self):
        detector_ids = self._model.pickable_detector_ids[:2]

        self._presenter.pick_detectors(detector_ids, sum_spectra=True)

        np.testing.assert_array_equal(np.sort(self._model.picked_detector_ids), np.sort(detector_ids))
        self.assertIsNotNone(self._model.line_plot_workspace)
        self._view.plot_spectra.assert_called_once_with(self._model.line_plot_workspace, True)

    def test_pick_detectors_with_unknown_id_does_not_change_the_selection(self):
        unknown_id = int(self._model.pickable_detector_ids.max()) + 1000

        with self.assertWarns(UserWarning):
            result = self._presenter.pick_detectors([unknown_id], sum_spectra=True)

        self.assertIsNone(result)
        self.assertEqual(len(self._model.picked_detector_ids), 0)


if __name__ == "__main__":
    unittest.main()
