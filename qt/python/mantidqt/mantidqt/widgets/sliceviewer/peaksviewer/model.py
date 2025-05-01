# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# local imports
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceDisplayModel
from .representation.draw import draw_peak_representation
from .representation.painter import Painted

# 3rd party imports
from mantid.api import AnalysisDataService, IPeaksWorkspace
from mantid.kernel import logger, SpecialCoordinateSystem

# standard library
import numpy as np
from typing import List

# map coordinate system to correct Peak getter
FRAME_TO_PEAK_CENTER_ATTR = {
    SpecialCoordinateSystem.QLab: "getQLabFrame",
    SpecialCoordinateSystem.QSample: "getQSampleFrame",
    SpecialCoordinateSystem.HKL: "getHKL",
}


class PeaksViewerModel(TableWorkspaceDisplayModel):
    """View model for PeaksViewer
    Extends PeaksWorkspace functionality to include color selection
    """

    def __init__(self, peaks_ws: IPeaksWorkspace, fg_color: str, bg_color: str):
        """
        :param peaks_ws: A pointer to the PeaksWorkspace
        :param fg_color: Color of the glyphs marking the signal region
        :param bg_color: Color of the glyphs marking the background region
        """
        if not hasattr(peaks_ws, "getNumberPeaks"):
            raise ValueError("Expected a PeaksWorkspace type but found a {}".format(type(peaks_ws)))

        super().__init__(peaks_ws)
        self._peaks_ws_name = peaks_ws.name()
        self._fg_color = fg_color
        self._bg_color = bg_color
        self._representations: List[Painted] = []
        self._orientedLattice = None
        self._calcHKL = False

        if peaks_ws.sample().hasOrientedLattice():
            self._orientedLattice = peaks_ws.sample().getOrientedLattice()

    @property
    def bg_color(self):
        return self._bg_color

    @property
    def fg_color(self):
        return self._fg_color

    @property
    def peaks_workspace(self):
        return self.ws

    @property
    def orientedLattice(self):
        return self._orientedLattice

    def can_calculate_hkl(self, frame):
        return frame == SpecialCoordinateSystem.HKL and self.ws.sample().hasOrientedLattice()

    def set_calculate_hkl(self, calc_hkl):
        self._calcHKL = calc_hkl

    def get_peaks_workspace_name(self):
        return self._peaks_ws_name

    def clear_peak_representations(self):
        """
        Remove drawn peaks from the view
        """
        for peak in self._representations:
            if peak:
                peak.remove()
        self._representations.clear()

    def draw_peaks(self, slice_info, painter, frame):
        """
        Draw a list of Peaks on the display
        :param slice_info: Object describing current slicing information
        :param painter: A reference to the object that will draw to the screen
        :param frame: coordinate system of workspace
        """
        representations = []
        if slice_info.can_support_peak_overlay():
            frame_to_slice_fn = self._frame_to_slice_fn(frame)
            for peak in self.ws:
                if self._calcHKL and frame == SpecialCoordinateSystem.HKL:
                    peak_origin = self.orientedLattice.hklFromQ(peak.getQSampleFrame())
                else:
                    peak_origin = getattr(peak, frame_to_slice_fn)()
                peak_repr = draw_peak_representation(peak_origin, peak.getPeakShape(), slice_info, painter, self.fg_color, self.bg_color)
                representations.append(peak_repr)
        self._representations = representations

    def add_peak(self, pos, frame):
        """Add a peak to the workspace using the given position and frame"""
        self.peaks_workspace.addPeak(pos, frame)

    def delete_peak(self, pos, frame):
        r"""Delete the peak closest to the input position"""

        if self.peaks_workspace.getNumberPeaks() == 0:
            return

        frame_to_slice_fn = self._frame_to_slice_fn(frame)

        def unpack_pos(peak):
            v3d_vector = getattr(peak, frame_to_slice_fn)()
            return [v3d_vector.X(), v3d_vector.Y(), v3d_vector.Z()]

        positions = np.array([unpack_pos(peak) for peak in self.peaks_workspace])
        positions -= pos  # peak positions relative to the input position
        distances_squared = np.sum(positions * positions, axis=1)
        closest_peak_index = np.argmin(distances_squared)
        return self.peaks_workspace.removePeak(int(closest_peak_index))  # required cast from numpy.int64 to int

    def slicepoint(self, selected_index, slice_info, frame):
        """
        Return the value of the center in the slice dimension for the peak at the given index
        :param selected_index: Index of a peak in the table
        :param slice_info: Information on the current slice
        """
        frame_to_slice_fn = self._frame_to_slice_fn(frame)
        peak = self.ws.getPeak(selected_index)
        if self._calcHKL and frame == SpecialCoordinateSystem.HKL:
            xyz = self.orientedLattice.hklFromQ(peak.getQSampleFrame())
        else:
            xyz = getattr(peak, frame_to_slice_fn)()
        slicepoint = slice_info.slicepoint
        slicepoint[slice_info.z_index] = slice_info.transform(xyz)[2]

        return slicepoint

    def viewlimits(self, index):
        """
        Retrieve the view to limits to display the peak center at the given index. It is assumed
        that slice point has been updated so it contains this peak
        :param index: Index of peak in list
        """
        # Sometimes the integration volume may not intersect the slices of data
        if not self.has_representations_drawn() or self._representations[index] is None:
            return ((None, None), (None, None))

        return self._representations[index].viewlimits()

    def has_representations_drawn(self) -> bool:
        return bool(self._representations)

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


def create_peaksviewermodel(peaks_ws_name: str, fg_color: str, bg_color: str):
    """
    A factory function to create a PeaksViewerModel from the given workspace name. Used by
    the PeaksViewerColletionPresenter when appending a new PeaksWorkspace.
    :param peaks_ws_name: A str giving a workspace name that is expected to be a PeaksWorkspace
    :param fg_color: Color of the glyphs marking the signal region
    :param bg_color: Color of the glyphs marking the background region
    :return: A new PeaksViewerModel object
    :raises ValueError: if the workspace referred to by the name is not a PeaksWorkspace
    """
    return PeaksViewerModel(_get_peaksworkspace(peaks_ws_name), fg_color, bg_color)


# Private
def _get_peaksworkspace(name: str):
    """Return a handle to a PeaksWorkspace
    :param name: The string name of a workspace in the ADS that should be a PeaksWorkspace
    :raises ValueError: if the workspace exists but is not a PeaksWorkspace
    :raises KeyError: if the workspace does not exist
    :return: A workspace handle for a PeaksWorkspace
    """
    workspace = AnalysisDataService.Instance()[name]
    if not hasattr(workspace, "getNumberPeaks"):
        raise ValueError("Requested workspace {} is not a PeaksWorkspace. Type={}".format(name, type(workspace)))
    return workspace
