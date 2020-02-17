# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports
import unittest

# 3rdparty imports
from mantid.api import (AnalysisDataServiceImpl, IPeaksWorkspace, ITableWorkspace, MatrixWorkspace)
from mantid.py3compat.mock import create_autospec

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.workspaceselection import PeaksWorkspaceSelectorModel


class PeaksWorkspaceSelectorModelTest(unittest.TestCase):
    def test_empty_workspaces_gives_empty_names_list(self):
        mock_ads = create_autospec(AnalysisDataServiceImpl)
        mock_ads.getObjectNames.return_value = []
        model = PeaksWorkspaceSelectorModel(mock_ads)

        name_status = model.names_and_statuses()

        self.assertEqual([], name_status)

    def test_zero_peaks_workspaces_gives_empty_names_list(self):
        contents = {
            "matrix": create_autospec(MatrixWorkspace),
            "table": create_autospec(ITableWorkspace)
        }
        model = PeaksWorkspaceSelectorModel(self._create_mock_ads(contents))

        name_status = model.names_and_statuses()

        self.assertEqual([], name_status)

    def test_mixed_workspaces_gives_only_peaks_workspaces(self):
        peaks_names = ["peaks1", "peaks2"]
        contents = {
            "matrix": create_autospec(MatrixWorkspace),
            peaks_names[0]: create_autospec(IPeaksWorkspace),
            "table": create_autospec(ITableWorkspace),
            peaks_names[1]: create_autospec(IPeaksWorkspace),
        }
        model = PeaksWorkspaceSelectorModel(self._create_mock_ads(contents))

        name_status = model.names_and_statuses()

        for name, _ in name_status:
            self.assertTrue(name in peaks_names)

    def test_default_check_status_false(self):
        contents = {
            "peaks1": create_autospec(IPeaksWorkspace),
            "peaks2": create_autospec(IPeaksWorkspace)
        }
        model = PeaksWorkspaceSelectorModel(self._create_mock_ads(contents))

        name_status = model.names_and_statuses()

        for _, check_status in name_status:
            self.assertFalse(check_status)

    def test_supplied_names_checked_by_default(self):
        contents = {
            "peaks1": create_autospec(IPeaksWorkspace),
            "peaks2": create_autospec(IPeaksWorkspace)
        }
        checked = ["peaks2"]
        model = PeaksWorkspaceSelectorModel(self._create_mock_ads(contents), checked)

        name_status = model.names_and_statuses()

        self.assertTrue(("peaks1", False) in name_status)
        self.assertTrue(("peaks2", True) in name_status)

    # private
    def _create_mock_ads(self, contents):
        mock_ads = create_autospec(AnalysisDataServiceImpl)
        mock_ads.__getitem__.side_effect = contents.__getitem__
        mock_ads.getObjectNames.return_value = contents.keys()
        return mock_ads


if __name__ == '__main__':
    unittest.main()
