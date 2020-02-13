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
DEFAULT_MARKER_COLOR = 'red'


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
        diminfo = sliceinfo.indices
        slicepoint = sliceinfo.point[diminfo[2]]
        dimrange = sliceinfo.range
        slicedim_width = dimrange[1] - dimrange[0]
        info = []
        for peak in self.ws:
            qlab = peak.getQLabFrame()
            # TODO: transformation
            qframe = qlab
            x, y, z = qframe[diminfo[0]], qframe[diminfo[1]], qframe[diminfo[2]]
            info.append(
                create_peakrepresentation(x, y, z, slicepoint, slicedim_width, peak.getPeakShape(),
                                          self.marker_color))
        self._visible_peaks = info
        return info

    def take_peak_representations(self):
        """Return the current set of peaks and remove them from the model
        :return: A list of the current peaks
        """
        peaks = self._visible_peaks
        self._visible_peaks = []
        return peaks

    def update_peak_representations(self, sliceinfo):
        """
        Update the internal list of PeakRepresentation objects describing each Peak.
        :param sliceinfo: Object describing current slicing information
        """
        diminfo = sliceinfo.indices
        slicepoint = sliceinfo.point[diminfo[2]]
        dimrange = sliceinfo.range
        slicedim_width = dimrange[1] - dimrange[0]
        for peak in self.visible_peaks:
            peak.update_alpha(slicepoint, slicedim_width)

        return self.visible_peaks
