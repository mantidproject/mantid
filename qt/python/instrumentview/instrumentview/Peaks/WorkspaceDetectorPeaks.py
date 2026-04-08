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
        for det_id, peaks_for_spec in groupby(peaks, lambda x: x.detector_id):
            self.detector_peaks.append(DetectorPeaks(list(peaks_for_spec)))

    def get_positions(self, detector_positions, detector_ids):
        peaks_ids = np.array([p.detector_id for p in self.detector_peaks])
        if len(peaks_ids) == 0:
            return np.array([]), []
        # Use argsort + searchsorted for fast lookup. Using np.where(np.isin) does not
        # maintain the original order. It is faster to sort then search the sorted
        # array for matching spectrum numbers
        sorter = np.argsort(detector_ids)
        idx = sorter[np.searchsorted(detector_ids, peaks_ids, sorter=sorter)]
        return detector_positions[idx]

    def get_labels(self):
        return [p.label for p in self.detector_peaks]
