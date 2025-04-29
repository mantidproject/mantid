# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

import matplotlib as mpl

mpl.use("Agg")
from mantid.simpleapi import CreateSampleWorkspace, RenameWorkspace, DeleteWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.samplelogs.presenter import SampleLogs
from qtpy.QtWidgets import QApplication


@start_qapplication
class SampleLogsViewTest(unittest.TestCase, QtWidgetFinder):
    def test_deleted_on_close(self):
        ws = CreateSampleWorkspace()
        pres = SampleLogs(ws)
        self.assert_widget_created()
        pres.view.close()

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_workspace_updates(self):
        ws = CreateSampleWorkspace()
        pres = SampleLogs(ws)

        # check rename of workspace
        assert pres.view.windowTitle() == "ws sample logs"
        assert pres.model._workspace_name == "ws"
        RenameWorkspace("ws", "new_name")
        assert pres.view.windowTitle() == "new_name sample logs"
        assert pres.model._workspace_name == "new_name"

        # check the workspace replaced if same name
        ws2 = CreateSampleWorkspace(OutputWorkspace="new_name", BinWidth=1000)
        assert repr(pres.model.get_ws()) == repr(ws2)

        # delete workspace and check that the widget closes
        assert pres.view.isVisible()
        DeleteWorkspace("new_name")
        assert not pres.view.isVisible()

    def test_minimum_canvas_size(self):
        ws = CreateSampleWorkspace()
        pres = SampleLogs(ws)
        pres.view.show_plot_and_stats(True)
        pres.view.canvas.resize(-1, -1)
        self.assertEqual(0, pres.view.canvas.width())
        self.assertEqual(0, pres.view.canvas.height())
