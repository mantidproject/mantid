# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from qtpy.QtWidgets import QApplication

from mantid.simpleapi import CreateSampleWorkspace, LoadInstrument
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter


@start_qapplication
class InstrumentViewTest(unittest.TestCase, QtWidgetFinder):
    def test_window_deleted_correctly(self):
        ws = CreateSampleWorkspace()
        LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap=False)

        p = InstrumentViewPresenter(ws)
        self.assert_widget_created()

        p.close(ws.name())

        QApplication.sendPostedEvents()
        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("instr")
        self.assert_no_toplevel_widgets()

    def test_window_force_deleted_correctly(self):
        ws = CreateSampleWorkspace()
        LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap=False)

        p = InstrumentViewPresenter(ws)
        self.assert_widget_created()

        p.force_close()

        QApplication.sendPostedEvents()
        self.assertEqual(None, p.ads_observer)
        self.assert_widget_not_present("instr")
        self.assert_no_toplevel_widgets()
