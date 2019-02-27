from __future__ import (absolute_import, division, print_function)
import platform
import sys
import unittest

from qtpy import PYQT_VERSION
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import Qt, QMetaObject

from mantid import FrameworkManager
from mantidqt.utils.qt.testing.gui_window_test import GuiWindowTest
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowserBase


def on_ubuntu_or_darwin():
    return ('Ubuntu' in platform.platform() and sys.version[0] == '2' or
            sys.platform == 'darwin' and PYQT_VERSION[0] == '4')


@unittest.skipIf(on_ubuntu_or_darwin(), "Popups don't show on ubuntu with python 2. Unskip when switched to xvfb."
                                        "Qt4 has a bug on macs which is fixed in Qt5.")
class TestFitPropertyBrowser(GuiWindowTest):

    def create_widget(self):
        return FitPropertyBrowserBase()

    def start_setup_menu(self):
        self.click_button('button_Setup')
        return self.wait_for_popup()

    def start_find_peaks(self):
        self.trigger_action('action_FindPeaks')

    def start_manage_setup(self):
        a, pm = self.get_action('action_ManageSetup', get_menu=True)
        pm.setActiveAction(a)
        m = a.menu()
        m.show()
        return self.wait_for_popup()

    def start_load_from_string(self):
        self.trigger_action('action_LoadFromString')
        return self.wait_for_modal()

    def set_function_string_blah(self):
        self.set_function_string('blah')
        return self.wait_for_modal()

    def set_function_string_linear(self):
        self.set_function_string('name=LinearBackground')
        return self.wait_for_true(lambda: self.widget.sizeOfFunctionsGroup() == 3)

    def set_function_string(self, text):
        box = self.get_active_modal_widget()
        box.setTextValue(text)
        QMetaObject.invokeMethod(box, 'accept', Qt.QueuedConnection)

    def test_find_peaks_no_workspace(self):
        yield self.start_setup_menu()
        m = self.get_menu('menu_Setup')
        self.assertTrue(m.isVisible())
        self.start_find_peaks()
        yield self.wait_for_modal()
        box = self.get_active_modal_widget()
        self.assertEqual(box.text(), 'Workspace name is not set')
        box.close()

    def test_load_from_string_blah(self):
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        yield self.start_load_from_string()
        yield self.set_function_string_blah()
        box = self.get_active_modal_widget()
        self.assertEqual(box.text(),
                         "Unexpected exception caught:\n\nError in input string to FunctionFactory\nblah")
        box.close()

    def test_load_from_string_lb(self):
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        yield self.start_load_from_string()
        yield self.set_function_string_linear()
        a = self.widget.getFittingFunction()
        self.assertEqual(a, 'name=LinearBackground,A0=0,A1=0')
        self.assertEqual(self.widget.sizeOfFunctionsGroup(), 3)

    def test_copy_to_clipboard(self):
        self.widget.loadFunction('name=LinearBackground,A0=0,A1=0')
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        QApplication.clipboard().clear()
        self.trigger_action('action_CopyToClipboard')
        yield self.wait_for_true(lambda: QApplication.clipboard().text() != '')
        self.assertEqual(QApplication.clipboard().text(), 'name=LinearBackground,A0=0,A1=0')

    def test_clear_model(self):
        self.widget.loadFunction('name=LinearBackground,A0=0,A1=0')
        self.assertEqual(self.widget.sizeOfFunctionsGroup(), 3)
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        self.trigger_action('action_ClearModel')
        yield self.wait_for_true(lambda: self.widget.sizeOfFunctionsGroup() == 2)
        self.assertEqual(self.widget.sizeOfFunctionsGroup(), 2)


if __name__ == '__main__':
    unittest.main()
    FrameworkManager.clear()
