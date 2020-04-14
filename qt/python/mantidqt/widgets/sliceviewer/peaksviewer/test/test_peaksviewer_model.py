# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# std imports
import unittest
from unittest.mock import MagicMock, create_autospec, patch

# thirdparty imports
from mantid.api import MatrixWorkspace
from mantid.dataobjects import PeaksWorkspace

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.model import PeaksViewerModel, create_peaksviewermodel


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
    def test_successive_create_peaksviewermodel_use_different_fg_colors(
            self, mock_get_peaks_workspace):
        mock_get_peaks_workspace.return_value = MagicMock(spec=PeaksWorkspace)

        first_model = create_peaksviewermodel('test')
        second_model = create_peaksviewermodel('test')

        self.assertNotEqual(first_model.fg_color, second_model.fg_color)

    # -------------------------- Failure Tests --------------------------------
    def test_model_accepts_only_peaks_workspaces(self):
        self.assertRaises(ValueError, PeaksViewerModel, create_autospec(MatrixWorkspace), 'w',
                          '1.0')


if __name__ == '__main__':
    unittest.main()
