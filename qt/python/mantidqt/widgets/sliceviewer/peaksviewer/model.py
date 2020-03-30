# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# 3rd party imports
from mantid.api import AnalysisDataService, SpecialCoordinateSystem
from mantid.kernel import logger

# local imports
from mantidqt.widgets.workspacedisplay.table.model \
    import TableWorkspaceDisplayModel
from .representation import create_peakrepresentation

# constants
DEFAULT_MARKER_COLOR = 'red'
# map coordinate system to correct Peak getter
FRAME_TO_PEAK_CENTER_ATTR = {
    SpecialCoordinateSystem.QLab: 'getQLabFrame',
    SpecialCoordinateSystem.QSample: 'getQSampleFrame',
    SpecialCoordinateSystem.HKL: 'getHKL',
}


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

        super().__init__(peaks_ws)
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
        try:
            peak_center_getter = FRAME_TO_PEAK_CENTER_ATTR[sliceinfo.frame]
        except KeyError:
            logger.warning("Unknown frame {}. Assuming QLab.".format(sliceinfo.frame))
            peak_center_getter = FRAME_TO_PEAK_CENTER_ATTR[SpecialCoordinateSystem.QLab]

        info = []
        for peak in self.ws:
            qframe = getattr(peak, peak_center_getter)()
            x, y, z = qframe[diminfo[0]], qframe[diminfo[1]], qframe[diminfo[2]]
            info.append(
                create_peakrepresentation(x, y, z, slicepoint, slicedim_width, peak.getPeakShape(),
                                          self.marker_color))
        self._visible_peaks = info
        return info

    def peak_representation_at(self, index):
        """Return a reference to the peak at the given index
        :return: A single Peak from the table at the index
        :raises: IndexError if the index is out of range
        """
        return self._visible_peaks[index]

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


def create_peaksviewermodel(peaks_ws_name):
    """
    A factory function to create a PeaksViewerModel from the given workspace name
    :param peaks_ws_name: A str giving a workspace name that is expected to be a PeaksWorkspace
    :return: A new PeaksViewerModel object
    :raises ValueError: if the workspace referred to by the name is not a PeaksWorkspace
    """
    return PeaksViewerModel(_get_peaksworkspace(peaks_ws_name))


# Private
def _get_peaksworkspace(name):
    """Return a handle to a PeaksWorkspace
    :param name: The string name of a workspace in the ADS that should be a PeaksWorkspace
    :raises ValueError: if the workspace exists but is not a PeaksWorkspace
    :raises KeyError: if the workspace does not exist
    :return: A workspace handle for a PeaksWorkspace
    """
    workspace = AnalysisDataService.Instance()[name]
    if not hasattr(workspace, 'getNumberPeaks'):
        raise ValueError("Requested workspace {} is not a PeaksWorkspace. Type={}".format(
            name, type(workspace)))
    return workspace
