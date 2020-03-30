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
from unittest.mock import create_autospec

# thirdparty imports
from mantid.api import MatrixWorkspace
from mantid.dataobjects import PeaksWorkspace

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.model import PeaksViewerModel


class PeaksViewerModelTest(unittest.TestCase):

    # -------------------------- Success Tests --------------------------------
    def test_peaks_workspace_returns_same_workspace_given_to_model(self):
        peaks_workspace = create_autospec(PeaksWorkspace)
        model = PeaksViewerModel(peaks_workspace)

        self.assertEqual(peaks_workspace, model.peaks_workspace)

    def test_marker_color_returns_string_identifier(self):
        model = PeaksViewerModel(create_autospec(PeaksWorkspace))

        self.assertTrue(isinstance(model.marker_color, str))

    # -------------------------- Failure Tests --------------------------------
    def test_model_accepts_only_peaks_workspaces(self):
        self.assertRaises(ValueError, PeaksViewerModel, create_autospec(MatrixWorkspace))


if __name__ == '__main__':
    unittest.main()
