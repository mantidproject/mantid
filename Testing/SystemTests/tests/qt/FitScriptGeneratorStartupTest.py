# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting

from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateWorkspace
from mantidqt.utils.qt.testing import get_application
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder  # noqa: E402

from mantidqt.widgets.fitscriptgenerator import (FittingMode, FitScriptGeneratorModel, FitScriptGeneratorPresenter,
                                                 FitScriptGeneratorView)

from qtpy.QtWidgets import QApplication


class FitScriptGeneratorStartupTest(systemtesting.MantidSystemTest, QtWidgetFinder):
    """
    A system test for testing that the Fit Script Generator interface opens ok.
    """
    def __init__(self):
        super(FitScriptGeneratorStartupTest, self).__init__()

        self._app = get_application()

        self.ws_name = "WorkspaceName"
        test_workspace = CreateWorkspace(DataX=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
                                         DataY=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], NSpec=4, UnitX="Wavelength")
        AnalysisDataService.addOrReplace(self.ws_name, test_workspace)

        self.fsg_model = FitScriptGeneratorModel()
        self.fsg_view = FitScriptGeneratorView(None, FittingMode.SIMULTANEOUS, {"Minimizer": "Levenberg-Marquardt"})
        self.fsg_presenter = FitScriptGeneratorPresenter(self.fsg_view, self.fsg_model, [self.ws_name], 1.0, 3.0)

    def cleanup(self):
        self.fsg_presenter = None
        self.fsg_view = None
        self.fsg_model = None
        self._app = None
        AnalysisDataService.clear()

    def runTest(self):
        try:
            self.fsg_presenter.openFitScriptGenerator()
            QApplication.sendPostedEvents()

            self.assert_widget_created()

            self.assertTrue(self.fsg_view.close())
            QApplication.sendPostedEvents()
        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open the Fit Script Generator interface: {ex}.")
