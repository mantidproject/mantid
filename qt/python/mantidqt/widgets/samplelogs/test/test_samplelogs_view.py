# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import

import unittest

import matplotlib as mpl
mpl.use('Agg')  # noqa
from mantid.simpleapi import CreateSampleWorkspace
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

        QApplication.processEvents()

        self.assert_no_toplevel_widgets()
