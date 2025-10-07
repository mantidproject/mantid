# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.Peaks.Peak import Peak


class DetectorPeaks:
    def __init__(self, peaks: list[Peak]) -> None:
        self.peaks = peaks
        self.detector_id = peaks[0].detector_id
        self.location = peaks[0].location
        self.label = ",".join([p.label for p in self.peaks])
