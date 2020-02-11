# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# local imports
from mantidqt.widgets.workspacedisplay.table.model \
    import TableWorkspaceDisplayModel
from .representation import create_peakrepresentation

# constants
DEFAULT_MARKER_COLOR = 'white'


class PeaksViewerModel(TableWorkspaceDisplayModel):
    """View model for PeaksViewer
    Extends PeaksWorkspace functionality to include color selection
    """

    def __init__(self, peaks_ws):
        """
        :param peaks_ws: A pointer to the PeaksWorkspace
        """
        if not hasattr(peaks_ws, 'getNumberPeaks'):
            raise ValueError("Expected a PeaksWorkspace type but found a {}".format(type(peaks_ws)))

        super(PeaksViewerModel, self).__init__(peaks_ws)
        self._marker_color = DEFAULT_MARKER_COLOR
        self._visible_peaks = []

    @property
    def marker_color(self):
        return self._marker_color

    @property
    def peaks_workspace(self):
        return self.ws

    @property
    def visible_peaks(self):
        return self._visible_peaks

    def compute_peak_representations(self, sliceinfo):
        """
        Create a list of PeakRepresentation object describing each Peak.
        :param sliceinfo: Object describing current slicing information
        """
        slicepoint, slicerange = sliceinfo.get_slicepoint()[2], sliceinfo.get_slicerange()
        info = []
        for peak in self.ws:
            info.append(create_peakrepresentation(peak, slicepoint, slicerange, self.marker_color))
        self._visible_peaks = info
        return info

    def update_peak_representations(self, sliceinfo):
        """
        Update the internal list of PeakRepresentation objects describing each Peak.
        :param sliceinfo: Object describing current slicing information
        """
        slicepoint, slicerange = sliceinfo.get_slicepoint()[2], sliceinfo.get_slicerange()
        for peak in self.visible_peaks:
            peak.update_alpha(slicepoint, slicerange)

        return self.visible_peaks
