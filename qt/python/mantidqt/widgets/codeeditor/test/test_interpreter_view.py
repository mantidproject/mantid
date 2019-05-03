# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

from mantidqt.utils.qt.testing import GuiTest
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.codeeditor.interpreter import PythonFileInterpreter


class PythonFileInterpreterViewTest(GuiTest, QtWidgetFinder):
    def test_find_replace_dialog(self):
        w = PythonFileInterpreter()

        # dialog not initialised by default, only when used
        self.assertIsNone(w.find_replace_dialog)

        w.show_find_replace_dialog()
        self.assertIsNotNone(w.find_replace_dialog)

        w.hide_find_replace_dialog()
        # dialog not deleted on hide - just hidden
        self.assertIsNotNone(w.find_replace_dialog)
        self.assertFalse(w.find_replace_dialog.visible)
