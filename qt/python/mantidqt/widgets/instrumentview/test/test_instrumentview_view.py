# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from qtpy.QtWidgets import QApplication

from mantid.simpleapi import CreateSampleWorkspace, LoadInstrument
from mantidqt.utils.qt.test import GuiTest
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter
from mantidqt.widgets.workspacedisplay.test_helper.qt_widget_finder import QtWidgetFinder


class InstrumentViewTest(GuiTest, QtWidgetFinder):
    def test_window_deleted_correctly(self):
        ws = CreateSampleWorkspace()
        LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap=False)

        p = InstrumentViewPresenter(ws)
        self.assert_window_created()

        p.close(ws.name())

        QApplication.processEvents()
        self.assertEqual(None, p.ads_observer)
        self.find_qt_widget("instr")

    def test_window_force_deleted_correctly(self):
        ws = CreateSampleWorkspace()
        LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap=False)

        p = InstrumentViewPresenter(ws)
        self.assert_window_created()

        p.force_close()

        QApplication.processEvents()
        self.assertEqual(None, p.ads_observer)
        self.find_qt_widget("instr")
