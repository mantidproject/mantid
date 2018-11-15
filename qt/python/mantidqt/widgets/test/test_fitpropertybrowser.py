from __future__ import (absolute_import, division, print_function)
import unittest

from qtpy.QtWidgets import QMessageBox, QApplication
from qtpy.QtCore import Qt, QMetaObject

from mantid import FrameworkManager
from mantidqt.utils.qt.test.test_window import GuiTestBase
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser


class TestFitPropertyBrowser(GuiTestBase):

    def __init__(self, test):
        super(TestFitPropertyBrowser, self).__init__(FitPropertyBrowser)
        self.test = test

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
        self.test.assertTrue(m.isVisible())
        yield self.start_find_peaks()
        box = self.get_active_modal_widget()
        self.test.assertTrue(isinstance(box, QMessageBox))
        self.test.assertEqual(box.text(), 'Workspace name is not set')
        box.close()

    def test_load_from_string_blah(self):
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        yield self.start_load_from_string()
        yield self.set_function_string_blah()
        box = self.get_active_modal_widget()
        self.test.assertEqual(box.text(),
                              "Unexpected exception caught:\n\nError in input string to FunctionFactory\nblah")
        box.close()

    def test_load_from_string_lb(self):
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        yield self.start_load_from_string()
        yield self.set_function_string_linear()
        a = self.widget.getFittingFunction()
        self.test.assertEqual(a, 'name=LinearBackground,A0=0,A1=0')
        self.test.assertEqual(self.widget.sizeOfFunctionsGroup(), 3)

    def test_copy_to_clipboard(self):
        self.widget.loadFunction('name=LinearBackground,A0=0,A1=0')
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        QApplication.clipboard().clear()
        self.trigger_action('action_CopyToClipboard')
        yield self.wait_for_true(lambda: QApplication.clipboard().text() != '')
        self.test.assertEqual(QApplication.clipboard().text(), 'name=LinearBackground,A0=0,A1=0')

    def test_clear_model(self):
        self.widget.loadFunction('name=LinearBackground,A0=0,A1=0')
        self.test.assertEqual(self.widget.sizeOfFunctionsGroup(), 3)
        yield self.start_setup_menu()
        yield self.start_manage_setup()
        self.trigger_action('action_ClearModel')
        yield self.wait_for_true(lambda: self.widget.sizeOfFunctionsGroup() == 2)
        self.test.assertEqual(self.widget.sizeOfFunctionsGroup(), 2)


class TestModalTester(unittest.TestCase):

    def test_find_peaks_no_workspace(self):
        TestFitPropertyBrowser(self).run('test_find_peaks_no_workspace', pause=0.)

    def test_load_from_string_blah(self):
        TestFitPropertyBrowser(self).run('test_load_from_string_blah', pause=0.)

    def test_load_from_string_lb(self):
        TestFitPropertyBrowser(self).run('test_load_from_string_lb', pause=0.)

    def test_copy_to_clipboard(self):
        TestFitPropertyBrowser(self).run('test_copy_to_clipboard', pause=0.)

    def test_clear_model(self):
        TestFitPropertyBrowser(self).run('test_clear_model', pause=0.)


if __name__ == '__main__':
    unittest.main()
