# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# std imports
import unittest
from unittest.mock import MagicMock, create_autospec, patch

# thirdparty imports
from mantid.api import MatrixWorkspace, SpecialCoordinateSystem
from mantid.dataobjects import PeaksWorkspace
from numpy.testing import assert_allclose

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.model import PeaksViewerModel, create_peaksviewermodel
from mantidqt.widgets.sliceviewer.peaksviewer.test.modeltesthelpers import create_peaks_viewer_model, draw_peaks  # noqa


class PeaksViewerModelTest(unittest.TestCase):

    # -------------------------- Success Tests --------------------------------
    def test_peaks_workspace_returns_same_workspace_given_to_model(self):
        peaks_workspace = create_autospec(PeaksWorkspace)
        model = PeaksViewerModel(peaks_workspace, 'b', '1.0')

        self.assertEqual(peaks_workspace, model.peaks_workspace)

    def test_color_returns_string_identifier_given_to_model(self):
        fg_color, bg_color = 'b', '0.5'
        model = PeaksViewerModel(create_autospec(PeaksWorkspace), fg_color, bg_color)

        self.assertEqual(fg_color, model.fg_color)
        self.assertEqual(bg_color, model.bg_color)

    @patch('mantidqt.widgets.sliceviewer.peaksviewer.model._get_peaksworkspace')
    def test_create_peaksviewermodel_uses_given_colors(self, mock_get_peaks_workspace):
        mock_get_peaks_workspace.return_value = MagicMock(spec=PeaksWorkspace)

        first_model = create_peaksviewermodel('test', 'red', 'gray')
        second_model = create_peaksviewermodel('test', 'blue', 'white')

        self.assertEqual('red', first_model.fg_color)
        self.assertEqual('gray', first_model.bg_color)
        self.assertEqual('blue', second_model.fg_color)
        self.assertEqual('white', second_model.bg_color)

    def test_draw_peaks(self):
        fg_color = 'r'
        # create 2 peaks: 1 visible, 1 not (far outside Z range)
        visible_peak_center, invisible_center = (0.5, 0.2, 0.25), (0.4, 0.3, 25)

        _, mock_painter = draw_peaks(
            (visible_peak_center, invisible_center), fg_color, slice_value=0.5, slice_width=30)

        self.assertEqual(1, mock_painter.cross.call_count)
        call_args, call_kwargs = mock_painter.cross.call_args
        self.assertEqual(visible_peak_center[0], call_args[0])
        self.assertEqual(visible_peak_center[1], call_args[1])
        self.assertAlmostEqual(0.03, call_args[2], places=3)
        self.assertAlmostEqual(0.356, call_kwargs["alpha"], places=3)
        self.assertEqual(fg_color, call_kwargs["color"])

    def test_clear_peaks_removes_all_drawn(self):
        # create 2 peaks: 1 visible, 1 not (far outside Z range)
        visible_peak_center, invisible_center = (0.5, 0.2, 0.25), (0.4, 0.3, 25)
        model, mock_painter = draw_peaks(
            (visible_peak_center, invisible_center), fg_color='r', slice_value=0.5, slice_width=30)

        model.clear_peak_representations()

        mock_painter.remove.assert_called_once()

    def test_slicepoint_transforms_center_to_correct_frame_and_order(self):
        peak_center = (1, 2, 3)
        model = create_peaks_viewer_model(centers=[peak_center], fg_color="red")
        slice_info = MagicMock()
        slice_info.slicepoint = [0.5, None, None]
        slice_info.z_index = 0
        slice_info.frame = SpecialCoordinateSystem.QSample

        slicepoint = model.slicepoint(0, slice_info)

        peak0 = model.ws.getPeak(0)
        peak0.getQSampleFrame.assert_called_once()
        peak0.getQLabFrame.assert_not_called()
        peak0.getHKL.assert_not_called()
        self.assertEqual([1, None, None], slicepoint)

    def test_viewlimits(self):
        visible_peak_center, invisible_center = (0.5, 0.2, 0.25), (0.4, 0.3, 25)
        model, mock_painter = draw_peaks(
            (visible_peak_center, invisible_center), fg_color='r', slice_value=0.5, slice_width=30)

        xlim, ylim = model.viewlimits(0)

        assert_allclose((-0.13, 1.13), xlim)
        assert_allclose((-0.43, 0.83), ylim)

    # -------------------------- Failure Tests --------------------------------
    def test_model_accepts_only_peaks_workspaces(self):
        self.assertRaises(ValueError, PeaksViewerModel, create_autospec(MatrixWorkspace), 'w',
                          '1.0')


if __name__ == '__main__':
    unittest.main()
