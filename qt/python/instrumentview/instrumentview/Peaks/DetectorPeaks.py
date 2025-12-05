# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.Peak import Peak


class DetectorPeaks:
    def __init__(self, peaks: list[Peak]) -> None:
        if not peaks:
            raise ValueError("peaks list cannot be empty")
        self.peaks = peaks
        self.detector_id = peaks[0].detector_id
        self.spectrum_no = peaks[0].spectrum_no
        self.location = peaks[0].location
        if len(self.peaks) > 1:
            # Find peak with highest d-spacing for label
            peak_d = max(self.peaks, key=lambda p: p.dspacing)
            self.label = peak_d.label.replace("(", "[").replace(")", "]") + f" x {len(self.peaks)}"
        else:
            self.label = peaks[0].label
