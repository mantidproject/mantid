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
    def __init__(self, pws_name):
        pws = AnalysisDataService.retrieve(pws_name)
        self.detector_peaks = []
        peaks = []
        peaks_dict = pws.toDict()
        detector_ids = peaks_dict["DetID"]
        hkls = zip(peaks_dict["h"], peaks_dict["k"], peaks_dict["l"], strict=True)
        tofs = peaks_dict["TOF"]
        dspacings = peaks_dict["DSpacing"]
        wavelengths = peaks_dict["Wavelength"]
        peaks += [
            Peak(det_id, peak_idx, hkl, tof, dspacing, wavelength, 2 * np.pi / dspacing)
            for (det_id, peak_idx, hkl, tof, dspacing, wavelength) in zip(
                detector_ids, range(len(tofs)), hkls, tofs, dspacings, wavelengths, strict=True
            )
        ]
        # groupby groups consecutive matches, so must be sorted
        peaks.sort(key=lambda x: x.detector_id)
        for _, peaks_for_spec in groupby(peaks, lambda x: x.detector_id):
            self.detector_peaks.append(DetectorPeaks(list(peaks_for_spec)))

    def get_positions_and_labels(self, detector_positions, detector_ids) -> tuple[np.ndarray, list]:
        peaks_ids = np.array([p.detector_id for p in self.detector_peaks])
        if len(peaks_ids) == 0:
            return np.array([]), []

        # Use argsort + searchsorted for fast lookup. Using np.where(np.isin) does not
        # maintain the original order. It is faster to sort then search the sorted
        # array for matching spectrum numbers
        sorter = np.argsort(detector_ids)
        sorted_detector_ids = detector_ids[sorter]
        positions = np.searchsorted(sorted_detector_ids, peaks_ids)
        ordered_indices = sorter[positions]
        valid = sorted_detector_ids[positions] == peaks_ids
        ordered_indices = ordered_indices[valid]
        labels = [p.label for p in np.array(self.detector_peaks)[valid]]
        return detector_positions[ordered_indices], labels

    def get_x_values_and_labels(self, unit, picked_detector_ids) -> tuple[list, list]:
        # x values for vertical markers in lineplot
        picked_peaks = [p for peak in self.detector_peaks for p in peak.peaks if peak.detector_id in picked_detector_ids]
        x_values = [p.location_in_unit(unit) for p in picked_peaks]
        labels = [p.label for p in picked_peaks]
        return x_values, labels
