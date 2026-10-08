# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from qtpy.QtWidgets import QAction, QApplication, QInputDialog, QMessageBox
from qtpy.QtCore import QTimer

from mantid import FrameworkManager
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowserBase


@start_qapplication
class TestFitPropertyBrowser(unittest.TestCase):
    def create_widget(self):
        return FitPropertyBrowserBase()

    def trigger_action(self, browser, name):
        browser.findChild(QAction, name).trigger()

    def when_modal_appears(self, dialog_type, handler, attempts=100):
        """Run handler on the next modal dialog of dialog_type. The dialog's exec() blocks the caller,
        so this must be scheduled before the action that opens it and is run by the dialog's event loop."""

        def check(remaining):
            dialog = QApplication.activeModalWidget()
            if isinstance(dialog, dialog_type):
                handler(dialog)
            elif remaining > 0:
                QTimer.singleShot(10, lambda: check(remaining - 1))

        QTimer.singleShot(0, lambda: check(attempts))

    def close_message_box(self, messages):
        def handler(box):
            messages.append(box.text())
            box.close()

        self.when_modal_appears(QMessageBox, handler)

    def enter_function_string(self, text):
        def handler(dialog):
            dialog.setTextValue(text)
            dialog.accept()

        self.when_modal_appears(QInputDialog, handler)

    def test_find_peaks_no_workspace(self):
        property_browser = self.create_widget()
        messages = []
        self.close_message_box(messages)

        self.trigger_action(property_browser, "action_FindPeaks")

        self.assertEqual(messages, ["Workspace name is not set"])

    def test_load_from_string_blah(self):
        property_browser = self.create_widget()
        messages = []
        self.enter_function_string("blah")
        self.close_message_box(messages)

        self.trigger_action(property_browser, "action_LoadFromString")

        self.assertEqual(messages, ["Unexpected exception caught:\n\nError in input string to FunctionFactory\nblah"])

    def test_load_from_string_lb(self):
        property_browser = self.create_widget()
        self.enter_function_string("name=LinearBackground")

        self.trigger_action(property_browser, "action_LoadFromString")

        self.assertEqual(property_browser.getFittingFunction(), "name=LinearBackground,A0=0,A1=0")
        self.assertEqual(property_browser.sizeOfFunctionsGroup(), 3)

    def test_multiple_function_string_loaded_correctly(self):
        property_browser = self.create_widget()
        func = (
            "name=Gaussian,Height=100,PeakCentre=1.45,Sigma=0.2,ties=(PeakCentre=1.45);name=Gaussian,Height=100,"
            "PeakCentre=7.5,Sigma=0.2,constraints=(0.18<Sigma<0.22),ties=(PeakCentre=7.5);"
            "ties=(f0.Sigma=f1.Sigma,f1.Height=f0.Height)"
        )

        property_browser.loadFunction(func)

        # tests composite func set correctly in browser (string incl. ties and constraints)
        self.assertEqual(func, property_browser.getFunctionString())
        for prefix in property_browser.getPeakPrefixes():
            h = property_browser.getPeakHandler(prefix)
            # check that the ties (as opposed to fixes) have been set on the child function property handlers
            # note that the non-fix tie string lives on the composite function but the properties whereas
            # the tie properties (m_ties in the C++ class) are on the child's handler
            self.assertTrue(h.hasTies())
            # check the peak centre is fixed
            self.assertTrue(h.ifun().isFixed(1))
        # check constraints on last function have correct length
        self.assertEqual(15, len(h.ifun().getConstraints()))

    def test_single_function_string_loaded_correctly(self):
        property_browser = self.create_widget()
        func = "name=Gaussian,Height=487,PeakCentre=5,Sigma=5;ties=(f0.Sigma=f0.PeakCentre)"

        property_browser.loadFunction(func)

        # test composite func set correctly in browser (string incl. ties and constraints)
        # note property_browser.getFunctionString() returns the child function (not composite) if only one function
        self.assertEqual(func, str(property_browser.currentHandler().ifun()))
        for prefix in property_browser.getPeakPrefixes():
            h = property_browser.getPeakHandler(prefix)
            self.assertTrue(h.hasTies())

    def test_copy_to_clipboard(self):
        property_browser = self.create_widget()
        property_browser.loadFunction("name=LinearBackground,A0=0,A1=0")
        QApplication.clipboard().clear()

        self.trigger_action(property_browser, "action_CopyToClipboard")

        self.assertEqual(QApplication.clipboard().text(), "name=LinearBackground,A0=0,A1=0")

    def test_clear_model(self):
        property_browser = self.create_widget()
        property_browser.loadFunction("name=LinearBackground,A0=0,A1=0")
        self.assertEqual(property_browser.sizeOfFunctionsGroup(), 3)

        self.trigger_action(property_browser, "action_ClearModel")

        self.assertEqual(property_browser.sizeOfFunctionsGroup(), 2)


if __name__ == "__main__":
    unittest.main()
    FrameworkManager.clear()
