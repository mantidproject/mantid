# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from mantid.api import WorkspaceFactory
from unittest import mock
from mantid.simpleapi import ExtractSpectra
from mantidqt.dialogs.spectraselectorutils import get_spectra_selection
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog


@start_qapplication
class SpectraSelectionUtilsTest(unittest.TestCase):
    _mock_get_icon = None
    _single_spec_ws = None
    _multi_spec_ws = None

    def setUp(self):
        # patch away getting a real icon as it can hit a race condition when running tests
        # in parallel
        patcher = mock.patch('mantidqt.dialogs.spectraselectordialog.get_icon')
        self._mock_get_icon = patcher.start()
        self._mock_get_icon.return_value = QIcon()
        self.addCleanup(patcher.stop)
        if self._single_spec_ws is None:
            self.__class__._single_spec_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=1,
                                                                                XLength=1, YLength=1)
            self.__class__._multi_spec_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=200,
                                                                               XLength=1, YLength=1)

    @mock.patch('mantidqt.dialogs.spectraselectorutils.SpectraSelectionDialog', autospec=True)
    def test_get_spectra_selection_cancelled_returns_None(self, mock_SpectraSelectionDialog):
        # a new instance of the mock created inside get_user_action_and_selection will return
        # dialog_mock
        mock_SpectraSelectionDialog.return_value = mock_SpectraSelectionDialog
        mock_SpectraSelectionDialog.Rejected = QDialog.Rejected
        mock_SpectraSelectionDialog.exec_.return_value = mock_SpectraSelectionDialog.Rejected
        mock_SpectraSelectionDialog.decision = QDialog.Rejected
        mock_SpectraSelectionDialog.selection = None
        mock_SpectraSelectionDialog.get_compatible_workspaces.return_value = [self._multi_spec_ws]

        selection = get_spectra_selection([self._multi_spec_ws])

        mock_SpectraSelectionDialog.exec_.assert_called_once_with()
        self.assertTrue(selection is None)

    @mock.patch('mantidqt.dialogs.spectraselectorutils.SpectraSelectionDialog')
    def test_get_spectra_selection_does_not_use_dialog_for_single_spectrum(self, dialog_mock):
        dialog_mock.get_compatible_workspaces.return_value = [self._single_spec_ws]
        selection = get_spectra_selection([self._single_spec_ws])

        dialog_mock.assert_not_called()
        self.assertEqual([0], selection.wksp_indices)
        self.assertEqual([self._single_spec_ws], selection.workspaces)

    @mock.patch('mantidqt.dialogs.spectraselectorutils.SpectraSelectionDialog')
    def test_get_spectra_selection_does_not_use_dialog_for_multiple__single_spectrum(self, dialog_mock):
        spectra_1 = ExtractSpectra(InputWorkspace=self._multi_spec_ws, StartWorkspaceIndex=0, EndWorkspaceIndex=0)
        spectra_2 = ExtractSpectra(InputWorkspace=self._multi_spec_ws, StartWorkspaceIndex=1, EndWorkspaceIndex=1)
        dialog_mock.get_compatible_workspaces.return_value = [spectra_1, spectra_2]
        selection = get_spectra_selection([spectra_1, spectra_2])

        dialog_mock.assert_not_called()
        self.assertEqual([0], selection.wksp_indices)
        self.assertEqual([spectra_1, spectra_2], selection.workspaces)


if __name__ == '__main__':
    unittest.main()
