# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# 3rd party imports
from cycler import cycler
from mantid.api import AnalysisDataService, SpecialCoordinateSystem
from mantid.kernel import logger

# local imports
from mantidqt.widgets.workspacedisplay.table.model \
    import TableWorkspaceDisplayModel
from .representation.draw import draw_peak_representation

# constants
# cycle colors for each peaks workspace - it is unlikely there will be more than 3
FG_COLORS = cycler(fg_color=[
    '#d62728',  # ~red,
    '#03ad06',  # ~green
    '#17becf',  # ~cyan
    '#e9f02e',  # ~yellow
    '#e377c2'  # ~pink
])()
DEFAULT_BG_COLOR = '0.75'
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

    def __init__(self, peaks_ws, fg_color, bg_color):
        """
        :param peaks_ws: A pointer to the PeaksWorkspace
        :param fg_color: Color of the glyphs marking the signal region
        :param bg_color: Color of the glyphs marking the background region
        """
        if not hasattr(peaks_ws, 'getNumberPeaks'):
            raise ValueError("Expected a PeaksWorkspace type but found a {}".format(type(peaks_ws)))

        super().__init__(peaks_ws)
        self._fg_color = fg_color
        self._bg_color = bg_color
        self._representations = []

    @property
    def bg_color(self):
        return self._bg_color

    @property
    def fg_color(self):
        return self._fg_color

    @property
    def peaks_workspace(self):
        return self.ws

    def clear_peak_representations(self):
        """
        Remove drawn peaks from the view
        """
        for peak in self._representations:
            if peak:
                peak.remove()
        self._representations.clear()

    def draw_peaks(self, slice_info, painter):
        """
        Draw a list of Peaks on the display
        :param slice_info: Object describing current slicing information
        :param painter: A reference to the object that will draw to the screen
        """
        frame_to_slice_fn = self._frame_to_slice_fn(slice_info.frame)

        representations = []
        for peak in self.ws:
            peak_origin = getattr(peak, frame_to_slice_fn)()
            peak_repr = draw_peak_representation(peak_origin, peak.getPeakShape(), slice_info,
                                                 painter, self.fg_color, self.bg_color)
            representations.append(peak_repr)

        self._representations = representations

    def slice_center(self, selected_index, slice_info):
        """
        Return the value of the center in the slice dimension for the peak at the given index
        :param selected_index: Index of a peak in the table
        :param slice_info: Information on the current slice
        """
        frame_to_slice_fn = self._frame_to_slice_fn(slice_info.frame)
        peak = self.ws.getPeak(selected_index)
        return slice_info.transform(getattr(peak, frame_to_slice_fn)())[2]

    def zoom_to(self, index):
        """
        Set the view to display the peak center at the given index. It is assumed
        that slice point has been updated so it contains this peak
        :param index: Index of peak in list
        """
        self._representations[index].zoom_to()

    def _frame_to_slice_fn(self, frame):
        """
        Return the appropriate function to retrieve the peak coordinates in the given frame
        :param frame: The frame of the data workspace
        """
        try:
            peak_center_getter = FRAME_TO_PEAK_CENTER_ATTR[frame]
        except KeyError:
            logger.warning("Unknown frame {}. Assuming QLab.".format(frame))
            peak_center_getter = FRAME_TO_PEAK_CENTER_ATTR[SpecialCoordinateSystem.QLab]
        return peak_center_getter


def create_peaksviewermodel(peaks_ws_name):
    """
    A factory function to create a PeaksViewerModel from the given workspace name
    :param peaks_ws_name: A str giving a workspace name that is expected to be a PeaksWorkspace
    :return: A new PeaksViewerModel object
    :raises ValueError: if the workspace referred to by the name is not a PeaksWorkspace
    """
    return PeaksViewerModel(
        _get_peaksworkspace(peaks_ws_name),
        fg_color=next(FG_COLORS)['fg_color'],
        bg_color=DEFAULT_BG_COLOR)


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
