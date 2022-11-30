# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest
from unittest import mock
from numpy import sqrt
# 3rd party imports
from mantidqt.widgets.sliceviewer.cutviewer.representation.cut_representation import CutRepresentation

fpath = "mantidqt.widgets.sliceviewer.cutviewer.representation.cut_representation"


class LinePlotsTest(unittest.TestCase):
    @mock.patch(fpath + ".CutRepresentation.draw", autospec=True)
    def setUp(self, mock_draw):
        self.canvas = mock.MagicMock()
        self.notify_on_release = mock.MagicMock()
        xmin, xmax, ymin, ymax, thickness = -1, 1, 0, 2, 0.2
        self.cut_rep = CutRepresentation(self.canvas, self.notify_on_release, xmin, xmax, ymin, ymax, thickness)
        self.cut_rep.start = mock.MagicMock()
        self.cut_rep.end = mock.MagicMock()
        self.cut_rep.start.get_xdata.return_value = [xmin]
        self.cut_rep.start.get_ydata.return_value = [ymin]
        self.cut_rep.end.get_xdata.return_value = [xmax]
        self.cut_rep.end.get_ydata.return_value = [ymax]

    def test_get_start_end_points(self):
        xmin, xmax, ymin, ymax = self.cut_rep.get_start_end_points()
        self.assertEqual(xmin, -1)
        self.assertEqual(xmax, 1)
        self.assertEqual(ymin, 0)
        self.assertEqual(ymax, 2)

    def test_get_mid_point(self):
        xcen, ycen = self.cut_rep.get_mid_point()

        self.assertEqual(xcen, 0.0)
        self.assertEqual(ycen, 1.0)

    def test_get_perp_dir(self):
        vecx, vecy = self.cut_rep.get_perp_dir()

        self.assertAlmostEqual(vecx, 1/sqrt(2))
        self.assertAlmostEqual(vecy, -1/sqrt(2))

    @mock.patch(fpath + ".CutRepresentation.clear_lines", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.draw", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_motion_midpoint_selected(self, mock_is_valid, mock_draw, mock_clear):
        mock_is_valid.return_value = True
        self.cut_rep.current_artist = mock.MagicMock()
        self.cut_rep.current_artist.get_label.return_value = 'mid'
        self.cut_rep.current_artist.get_xdata.return_value = [0.0]
        mock_event = mock.MagicMock()
        mock_event.xdata = 4
        mock_event.ydata = 5

        self.cut_rep.on_motion(mock_event)

        mock_clear.assert_called_once()
        mock_draw.assert_called_once()
        self.cut_rep.start.set_data.assert_called_with([3.0], [4.0])
        self.cut_rep.end.set_data.assert_called_with([5.0], [6.0])

    @mock.patch(fpath + ".CutRepresentation.clear_lines", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.draw", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_motion_thickness_selected(self, mock_is_valid, mock_draw, mock_clear):
        mock_is_valid.return_value = True
        self.cut_rep.current_artist = mock.MagicMock()
        self.cut_rep.current_artist.get_label.return_value = 'mid_box_top'
        self.cut_rep.current_artist.get_xdata.return_value = [0.0]
        mock_event = mock.MagicMock()
        mock_event.xdata = 0.5
        mock_event.ydata = 0.5

        self.cut_rep.on_motion(mock_event)

        mock_clear.assert_called_once()
        mock_draw.assert_called_once()
        self.assertAlmostEqual(self.cut_rep.thickness, sqrt(2))

    @mock.patch(fpath + ".CutRepresentation.clear_lines", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.draw", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_motion_end_point_selected(self, mock_is_valid, mock_draw, mock_clear):
        mock_is_valid.return_value = True
        self.cut_rep.current_artist = mock.MagicMock()
        self.cut_rep.current_artist.get_label.return_value = 'end'
        self.cut_rep.current_artist.get_xdata.return_value = [0.0]
        mock_event = mock.MagicMock()
        mock_event.xdata = 2.0
        mock_event.ydata = 3.0

        self.cut_rep.on_motion(mock_event)

        mock_clear.assert_called_once()
        mock_draw.assert_called_once()
        self.cut_rep.current_artist.set_data.assert_called_once_with([2.0], [3.0])

    def test_is_valid_event_if_event_not_in_axes(self):
        mock_event = mock.MagicMock()
        mock_event.inaxes = None

        self.assertFalse(self.cut_rep.is_valid_event(mock_event))

    def test_is_valid_event_if_event_in_axes(self):
        mock_event = mock.MagicMock()
        mock_event.inaxes = self.cut_rep.ax

        self.assertTrue(self.cut_rep.is_valid_event(mock_event))

    def test_has_current_artist(self):
        self.cut_rep.current_artist = None

        self.assertFalse(self.cut_rep.has_current_artist())

    @mock.patch(fpath + ".CutRepresentation.has_current_artist", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_release_not_notify_if_not_valid_event(self, mock_is_valid, mock_has_artist):
        mock_is_valid.return_value = False
        mock_has_artist.return_value = True

        self.cut_rep.on_release("event")

        self.notify_on_release.assert_not_called()

    @mock.patch(fpath + ".CutRepresentation.has_current_artist", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_release_not_notify_if_no_current_artist(self, mock_is_valid, mock_has_artist):
        mock_is_valid.return_value = True
        mock_has_artist.return_value = False

        self.cut_rep.on_release("event")

        self.notify_on_release.assert_not_called()

    @mock.patch(fpath + ".CutRepresentation.has_current_artist", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_release_swaps_start_end_so_xmin_less_than_xmax(self, mock_is_valid, mock_has_artist):
        mock_is_valid.return_value = True
        mock_has_artist.return_value = True
        # overwrite end point to have x < start point
        self.cut_rep.end.get_xdata.return_value = [-2]

        self.cut_rep.on_release("event")

        self.notify_on_release.assert_called_with(-2, -1, 2, 0, 0.2)


if __name__ == '__main__':
    unittest.main()
