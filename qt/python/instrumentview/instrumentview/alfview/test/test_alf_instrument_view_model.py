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

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.alfview.ALFInstrumentViewModel import ALFInstrumentViewModel


class TestALFInstrumentViewModel(unittest.TestCase):
    def setUp(self):
        self._model = ALFInstrumentViewModel(MagicMock())

    def test_setup_delegates_to_parent_setup(self):
        with mock.patch.object(FullInstrumentViewModel, "setup") as mock_parent_setup:
            self._model.setup()

        mock_parent_setup.assert_called_once_with()

    def test_setup_restores_cached_picking(self):
        detector_ids = np.array([1, 2, 3], dtype=int)
        detector_is_picked = np.array([True, False, True], dtype=bool)
        point_picked_detectors = np.array([False, True, False], dtype=bool)
        current_detector_groupings = np.array([10, 20, 30], dtype=int)

        self._model._detector_ids = detector_ids.copy()
        self._model._detector_is_picked = detector_is_picked.copy()
        self._model._point_picked_detectors = point_picked_detectors.copy()
        self._model._current_detector_groupings = current_detector_groupings.copy()

        def fake_parent_setup():
            self._model._detector_ids = detector_ids.copy()
            self._model._detector_is_picked = np.zeros_like(detector_is_picked, dtype=bool)
            self._model._point_picked_detectors = np.zeros_like(point_picked_detectors, dtype=bool)
            self._model._current_detector_groupings = np.zeros_like(current_detector_groupings)

        with mock.patch.object(FullInstrumentViewModel, "setup", side_effect=fake_parent_setup):
            self._model.setup()

        np.testing.assert_array_equal(self._model._detector_ids, detector_ids)
        np.testing.assert_array_equal(self._model._detector_is_picked, detector_is_picked)
        np.testing.assert_array_equal(self._model._point_picked_detectors, point_picked_detectors)
        np.testing.assert_array_equal(self._model._current_detector_groupings, current_detector_groupings)

    def test_setup_does_not_restore_for_mismatched_shapes(self):
        self._model._detector_ids = np.array([1, 2, 3], dtype=int)
        self._model._detector_is_picked = np.array([True, False, True], dtype=bool)
        self._model._point_picked_detectors = np.array([False, True, False], dtype=bool)
        self._model._current_detector_groupings = np.array([10, 20, 30], dtype=int)

        def fake_parent_setup():
            self._model._detector_ids = np.array([1, 2], dtype=int)
            self._model._detector_is_picked = np.array([False, False], dtype=bool)
            self._model._point_picked_detectors = np.array([False, False], dtype=bool)
            self._model._current_detector_groupings = np.array([0, 0], dtype=int)

        with mock.patch.object(FullInstrumentViewModel, "setup", side_effect=fake_parent_setup):
            self._model.setup()

        np.testing.assert_array_equal(self._model._detector_is_picked, np.array([False, False], dtype=bool))
        np.testing.assert_array_equal(self._model._point_picked_detectors, np.array([False, False], dtype=bool))
        np.testing.assert_array_equal(self._model._current_detector_groupings, np.array([0, 0], dtype=int))


if __name__ == "__main__":
    unittest.main()
