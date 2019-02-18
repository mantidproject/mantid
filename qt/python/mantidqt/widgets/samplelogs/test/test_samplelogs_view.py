# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtWidgets import QApplication

from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.test import GuiTest
from mantidqt.utils.qt.test.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.samplelogs.presenter import SampleLogs


class SampleLogsViewTest(GuiTest, QtWidgetFinder):
    def test_deleted_on_close(self):
        ws = CreateSampleWorkspace()
        pres = SampleLogs(ws)
        self.assert_window_created()
        pres.view.close()

        QApplication.processEvents()

        self.assert_no_toplevel_widgets()
