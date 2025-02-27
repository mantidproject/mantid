# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""A collection of functions to share creating models for tests"""

# std imports
from unittest.mock import MagicMock, create_autospec

# 3rd party imports
from mantid.kernel import SpecialCoordinateSystem
from mantid.dataobjects import PeaksWorkspace
from mantid.kernel import V3D

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.model import PeaksViewerModel
from mantidqt.widgets.sliceviewer.peaksviewer.representation.painter import MplPainter


def draw_peaks(centers, fg_color, slice_value, slice_width, frame=SpecialCoordinateSystem.QLab):
    model = create_peaks_viewer_model(centers, fg_color)
    slice_info = create_slice_info(centers, slice_value, slice_width)
    mock_painter = MagicMock(spec=MplPainter)
    mock_axes = MagicMock()
    mock_axes.get_xlim.return_value = (-1, 1)
    mock_painter.axes = mock_axes

    model.draw_peaks(slice_info, mock_painter, frame)

    return model, mock_painter


def create_peaks_viewer_model(centers, fg_color, name=None):
    peaks = [create_mock_peak(center) for center in centers]

    def get_peak(index):
        return peaks[index]

    def column(name: str):
        if name in ("QLab", "QSample"):
            return centers

    def remove_peak(peak_number: int):
        return peak_number

    model = PeaksViewerModel(create_autospec(PeaksWorkspace, instance=True), fg_color, "unused")
    if name is not None:
        model.ws.name.return_value = name
    model.ws.__iter__.return_value = peaks
    model.ws.getPeak.side_effect = get_peak
    model.ws.column.side_effect = column
    model.ws.removePeak.side_effect = remove_peak

    return model


def create_mock_peak(center):
    peak = MagicMock()
    # set all 3 methods to return the same thing. Check appropriate method called in test
    peak.getQLabFrame.return_value = V3D(*center)
    peak.getQSampleFrame.return_value = V3D(*center)
    peak.getHKL.return_value = V3D(*center)
    shape = MagicMock()
    shape.shapeName.return_value = "none"
    peak.getPeakShape.return_value = shape
    return peak


def create_slice_info(transform_side_effect, slice_value, slice_width):
    slice_info = MagicMock()
    slice_info.transform.side_effect = transform_side_effect
    slice_info.z_value = slice_value
    slice_info.z_width = slice_width
    return slice_info
