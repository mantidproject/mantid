# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.DetectorPeaks import DetectorPeaks
from instrumentview.Peaks.Peak import Peak
from itertools import groupby
import numpy as np
from mantid.simpleapi import AnalysisDataService


class WorkspaceDetectorPeaks:
    def __init__(self, pws_name, unit, limits):
        self._detector_peaks = []
        # Read peaks from workspace in a given unit range
        pws = AnalysisDataService.retrieve(pws_name)
        peaks_dict = pws.toDict()
        detector_ids = peaks_dict["DetID"]
        hkls = zip(peaks_dict["h"], peaks_dict["k"], peaks_dict["l"], strict=True)
        tofs = peaks_dict["TOF"]
        dspacings = peaks_dict["DSpacing"]
        wavelengths = peaks_dict["Wavelength"]
        peaks = []
        for det_id, peak_idx, hkl, tof, dspacing, wavelength in zip(
            detector_ids, range(len(tofs)), hkls, tofs, dspacings, wavelengths, strict=True
        ):
            p = Peak(det_id, peak_idx, hkl, tof, dspacing, wavelength, 2 * np.pi / dspacing)
            if self._is_within_limits(p.location_in_unit(unit), limits):
                peaks.append(p)

        # groupby groups consecutive matches, so must be sorted
        peaks.sort(key=lambda x: x.detector_id)
        for _, peaks_for_spec in groupby(peaks, lambda x: x.detector_id):
            self._detector_peaks.append(DetectorPeaks(list(peaks_for_spec)))

    @property
    def detector_peaks(self):
        return self._detector_peaks

    def get_peaks_indices_and_labels(self, detector_ids) -> tuple[np.ndarray, list]:
        peaks_ids = np.array([p.detector_id for p in self._detector_peaks])
        if len(peaks_ids) == 0 or len(detector_ids) == 0:
            return np.array([], dtype=int), []

        # Use argsort + searchsorted for fast lookup. Using np.where(np.isin) does not
        # maintain the original order. It is faster to sort then search the sorted
        # array for matching spectrum numbers
        sorter = np.argsort(detector_ids)
        sorted_detector_ids = detector_ids[sorter]
        positions = np.searchsorted(sorted_detector_ids, peaks_ids)
        positions = np.clip(positions, 0, len(sorted_detector_ids) - 1)
        ordered_indices = sorter[positions]
        valid = sorted_detector_ids[positions] == peaks_ids
        ordered_indices = ordered_indices[valid]
        labels = [p.label for p in np.array(self._detector_peaks)[valid]]
        return ordered_indices, labels

    def get_x_values_and_labels(self, picked_detector_ids) -> list[Peak]:
        picked_peaks = [p for peak in self._detector_peaks for p in peak.peaks if peak.detector_id in picked_detector_ids]
        return picked_peaks

    def _is_within_limits(self, x, limits):
        return x >= min(limits) and x <= max(limits)
