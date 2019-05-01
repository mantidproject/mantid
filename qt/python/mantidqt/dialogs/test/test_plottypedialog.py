# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QApplication

from mantid.api import WorkspaceFactory
from mantid.py3compat import mock
from mantidqt.dialogs.plottypedialog import PlotTypeDialog
from mantidqt.utils.qt.testing import GuiTest


class PlotTypeDialogTest(GuiTest):
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

    def test_btn_spectrum_clicked_accepted(self):
        ptd = PlotTypeDialog()
        ptd.btn_spectra.click()
        QApplication.processEvents()

        self.assertEqual(PlotTypeDialog.Spectra, ptd.decision)

    def test_btn_colorfill_clicked(self):
        ptd = PlotTypeDialog()
        ptd.btn_colorfil.click()
        QApplication.processEvents()
        self.assertEqual(PlotTypeDialog.Colorfill, ptd.decision)

    def test_btn_spectrum_clicked_rejected(self):
        ptd = PlotTypeDialog()
        ptd.close()
        QApplication.processEvents()
        self.assertEqual(None, ptd.decision)
