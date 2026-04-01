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


class WorkspaceDetectorPeaks:
    projected_points: np.ndarray
    labels: list[str]

    def __init__(self, pws, colour, workspace, spectrum_nos):
        self.detector_peaks = []
        self.colour = colour
        self.name = pws.name()

        detector_info = workspace.detectorInfo()
        peaks = []
        peaks_dict = pws.toDict()
        detector_ids = peaks_dict["DetID"]
        workspace_indices = workspace.getIndicesFromDetectorIDs(detector_ids)
        spectrum_nos = spectrum_nos[workspace_indices]
        hkls = zip(peaks_dict["h"], peaks_dict["k"], peaks_dict["l"], strict=True)
        positions = [np.array(detector_info.position(detector_info.indexOf(id))) for id in detector_ids]
        tofs = peaks_dict["TOF"]
        dspacings = peaks_dict["DSpacing"]
        wavelengths = peaks_dict["Wavelength"]
        peaks += [
            Peak(det_id, spec_no, v, peak_idx, hkl, tof, dspacing, wavelength, 2 * np.pi / dspacing)
            for (det_id, spec_no, v, peak_idx, hkl, tof, dspacing, wavelength) in zip(
                detector_ids, spectrum_nos, positions, range(len(tofs)), hkls, tofs, dspacings, wavelengths, strict=True
            )
        ]
        # groupby groups consecutive matches, so must be sorted
        peaks.sort(key=lambda x: x.spectrum_no)
        for spec_no, peaks_for_spec in groupby(peaks, lambda x: x.spectrum_no):
            self.detector_peaks.append(DetectorPeaks(list(peaks_for_spec)))

    def set_positions_and_labels(self, detector_positions, spectrum_nos):
        peaks_spectrum_nos = np.array([p.spectrum_no for p in self.detector_peaks])
        if len(spectrum_nos) == 0:
            self.labels = []
            self.projected_points = np.empty((0, detector_positions.shape[1] if detector_positions.ndim > 1 else 0))
            return
        # Use argsort + searchsorted for fast lookup. Using np.where(np.isin) does not
        # maintain the original order. It is faster to sort then search the sorted
        # array for matching spectrum numbers
        sorted_idx = np.argsort(spectrum_nos)
        sorted_spectrum_nos = spectrum_nos[sorted_idx]
        positions = np.searchsorted(sorted_spectrum_nos, peaks_spectrum_nos)
        # Map back to original indices
        ordered_indices = sorted_idx[positions]
        valid = sorted_spectrum_nos[positions] == peaks_spectrum_nos
        ordered_indices = ordered_indices[valid]
        self.labels = [p.label for i, p in enumerate(self.detector_peaks) if valid[i]]
        self.projected_points = detector_positions[ordered_indices]
