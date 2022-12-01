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
        transform = mock.MagicMock()
        transform.tr = mock.MagicMock()
        transform.tr.side_effect = lambda x, y: (x, y)
        transform.inv_tr = mock.MagicMock()
        transform.inv_tr.side_effect = lambda x, y: (x, y)
        self.cut_rep = CutRepresentation(self.canvas, self.notify_on_release, xmin, xmax, ymin, ymax, thickness, transform)
        self.cut_rep.start = mock.MagicMock()
        self.cut_rep.ax.plot = mock.MagicMock()
        self.cut_rep.end = mock.MagicMock()
        self.cut_rep.start.get_xdata.return_value = [xmin]
        self.cut_rep.start.get_ydata.return_value = [ymin]
        self.cut_rep.end.get_xdata.return_value = [xmax]
        self.cut_rep.end.get_ydata.return_value = [ymax]

        xmin, xmax, ymin, ymax, thickness = -1, 1, 0, 2, 0.2
        transform_no = mock.MagicMock()
        transform_no.tr = mock.MagicMock()

        # rotate anticlockwise by 90 degrees and scale - code requires tranformation to leave origin unaltered
        def return_different(arg1, arg2):
            return -0.5 * arg2, 0.5 * arg1

        def return_different_inv(arg1, arg2):
            return 2 * arg2, -2 * arg1

        transform_no.tr.side_effect = return_different
        transform_no.inv_tr = mock.MagicMock()
        transform_no.inv_tr.side_effect = return_different_inv
        self.cut_rep_no = CutRepresentation(self.canvas, self.notify_on_release, xmin, xmax, ymin, ymax, thickness, transform_no)
        self.cut_rep_no.start = mock.MagicMock()
        self.cut_rep_no.ax.plot = mock.MagicMock()
        self.cut_rep_no.end = mock.MagicMock()
        self.cut_rep_no.start.get_xdata.return_value = [return_different(xmin, ymin)[0]]
        self.cut_rep_no.start.get_ydata.return_value = [return_different(xmin, ymin)[1]]
        self.cut_rep_no.end.get_xdata.return_value = [return_different(xmax, ymax)[0]]
        self.cut_rep_no.end.get_ydata.return_value = [return_different(xmax, ymax)[1]]

    def test_draw_line(self):
        self.cut_rep.draw_line()
        self.cut_rep.ax.plot.assert_any_call(-1, 0, 'ow', label='start')
        self.cut_rep.ax.plot.assert_any_call(1, 2, 'ow', label='end')
        self.cut_rep.ax.plot.assert_any_call(0, 1, label='mid', marker='o', color='w', markerfacecolor='w')

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
        self.assertEqual(self.cut_rep.xmin_c, 3.0)
        self.assertEqual(self.cut_rep.ymin_c, 4.0)
        self.assertEqual(self.cut_rep.xmax_c, 5.0)
        self.assertEqual(self.cut_rep.ymax_c, 6.0)

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
        self.assertAlmostEqual(self.cut_rep.thickness_c, sqrt(2))

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
        self.assertEqual(self.cut_rep.xmax_c, 2.0)
        self.assertEqual(self.cut_rep.ymax_c, 3.0)

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
        self.cut_rep.xmax_c= -2

        self.cut_rep.on_release("event")

        self.notify_on_release.assert_called_with(-2, -1, 2, 0, 0.2)

    def test_draw_line_non_orth(self):
        self.cut_rep_no.draw_line()
        self.cut_rep_no.ax.plot.assert_any_call(0, -0.5, 'ow', label='start')
        self.cut_rep_no.ax.plot.assert_any_call(-1, 0.5, 'ow', label='end')

    def test_get_start_end_points_non_orth(self):
        xmin, xmax, ymin, ymax = self.cut_rep_no.get_start_end_points()
        self.assertEqual(xmin, 0)
        self.assertEqual(xmax, -1)
        self.assertEqual(ymin, -0.5)
        self.assertEqual(ymax, 0.5)

    def test_get_perp_dir_non_ortho(self):
        vecx, vecy = self.cut_rep_no.get_perp_dir()

        self.assertAlmostEqual(vecx, 1/sqrt(2))
        self.assertAlmostEqual(vecy, 1/sqrt(2))

    def test_get_perp_dir_crystal_non_ortho(self):
        vecx, vecy = self.cut_rep_no.get_perp_dir_crystal()

        self.assertAlmostEqual(vecx, 1/sqrt(2))
        self.assertAlmostEqual(vecy, -1/sqrt(2))

    @mock.patch(fpath + ".CutRepresentation.clear_lines", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.draw", autospec=True)
    @mock.patch(fpath + ".CutRepresentation.is_valid_event", autospec=True)
    def test_on_motion_thickness_selected_non_ortho(self, mock_is_valid, mock_draw, mock_clear):
        mock_is_valid.return_value = True
        self.cut_rep_no.current_artist = mock.MagicMock()
        self.cut_rep_no.current_artist.get_label.return_value = 'mid_box_top'
        mock_event = mock.MagicMock()
        # cut is pointing top left to bottom right with gradient -1
        # Drag thickness out in top right direction from midpoint at (-0.5,0) towards point (-0.25, 0.25)
        mock_event.xdata = -0.25
        mock_event.ydata = 0.25

        self.cut_rep_no.on_motion(mock_event)

        mock_clear.assert_called_once()
        mock_draw.assert_called_once()
        self.assertAlmostEqual(self.cut_rep_no.thickness_c, sqrt(2))


if __name__ == '__main__':
    unittest.main()
