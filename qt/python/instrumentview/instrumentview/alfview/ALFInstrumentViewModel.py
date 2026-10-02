# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Optional

import numpy as np

from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel


class ALFInstrumentViewModel(FullInstrumentViewModel):
    # The sole purpose of this subclass is to preserve the picking state when the workspace is reset in ALFView.
    # So when new runs are loaded, the same picked banks remain selected.
    def setup(self):
        cached_picking_state = self._cache_picking_state()
        super().setup()
        self._restore_picking_state(cached_picking_state)

    def _cache_picking_state(self) -> Optional[tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]]:
        required_attributes = (
            "_detector_ids",
            "_detector_is_picked",
            "_point_picked_detectors",
            "_current_detector_groupings",
        )

        if any(not hasattr(self, attribute) for attribute in required_attributes):
            return None

        return (
            self._detector_ids.copy(),
            self._detector_is_picked.copy(),
            self._point_picked_detectors.copy(),
            self._current_detector_groupings.copy(),
        )

    def _restore_picking_state(self, cached_picking_state: Optional[tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]]) -> None:
        if cached_picking_state is None:
            return

        detector_ids, detector_is_picked, point_picked_detectors, current_detector_groupings = cached_picking_state
        if (
            detector_is_picked.shape == self._detector_is_picked.shape
            and point_picked_detectors.shape == self._point_picked_detectors.shape
            # Detector IDs should always match since alfview uses the same workspace for storing data
            and np.array_equal(detector_ids, self._detector_ids)
            and current_detector_groupings.shape == self._current_detector_groupings.shape
        ):
            self._detector_is_picked = detector_is_picked
            self._point_picked_detectors = point_picked_detectors
            self._current_detector_groupings = current_detector_groupings
