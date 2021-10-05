# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
#
import unittest

from qtpy import QtGui
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.workspacecalculator.presenter import WorkspaceCalculator
from mantidqt.widgets.workspacecalculator.view import ScaleValidator
from qtpy.QtWidgets import QApplication


@start_qapplication
class WorkspaceCalculatorViewTest(unittest.TestCase, QtWidgetFinder):
    def test_deleted_on_close(self):
        presenter = WorkspaceCalculator()
        self.assert_widget_created()
        presenter.view.close()
        QApplication.sendPostedEvents()
        self.assert_no_toplevel_widgets()

    def test_validator(self):
        scale_validator = ScaleValidator()
        self.assertTrue(scale_validator)
        self.assertEqual(scale_validator.validate('1.0', 1)[0], QtGui.QValidator.Acceptable)
        self.assertEqual(scale_validator.validate('-1.0', 1)[0], QtGui.QValidator.Acceptable)
        self.assertEqual(scale_validator.validate('1e3', 1)[0], QtGui.QValidator.Acceptable)
        self.assertEqual(scale_validator.validate('1e-3', 1)[0], QtGui.QValidator.Acceptable)
        self.assertEqual(scale_validator.validate('0', 1)[0], QtGui.QValidator.Intermediate)
        self.assertEqual(scale_validator.validate('not_a_number', 1)[0], QtGui.QValidator.Invalid)
