# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import

import unittest

from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from qtpy.QtWidgets import QApplication


@start_qapplication
class MatrixWorkspaceDisplayViewTest(unittest.TestCase, QtWidgetFinder):
    def test_window_deleted_correctly(self):
        ws = CreateSampleWorkspace()

        p = MatrixWorkspaceDisplay(ws)
        self.assert_widget_created()
        p.close(ws.name())

        self.assert_widget_created()

        QApplication.processEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()

    def test_window_force_deleted_correctly(self):
        ws = CreateSampleWorkspace()

        p = MatrixWorkspaceDisplay(ws)
        self.assert_widget_created()

        p.force_close()

        self.assert_widget_created()

        QApplication.processEvents()

        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("work")
        self.assert_no_toplevel_widgets()
