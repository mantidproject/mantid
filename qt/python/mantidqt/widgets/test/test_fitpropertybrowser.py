from __future__ import (absolute_import, division, print_function,
                        unicode_literals)
import unittest

from mantidqt.utils.qt.test.test_window import modals, GuiTestBase
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser


class TestModalTester(unittest.TestCase):

    def test_fit(test):

        @modals('start_dialog')
        class MyTest(GuiTestBase):

            def __init__(self):
                super(MyTest, self).__init__()
                self.button_display = None

            def call(self):
                self.start_dialog()
                m = self.get_menu('menu_Display')
                action = self.get_action('action_Quality')
                yield self.wait_for('button_display')
                test.assertTrue(m.isVisible())
                test.assertTrue(action.isChecked())
                self.trigger_action('action_Quality')
                yield
                test.assertFalse(m.isVisible())
                test.assertFalse(action.isChecked())

            def start_dialog(self):
                self.button_display = self.get_button('button_Display')
                self.button_display.click()

        MyTest.run(FitPropertyBrowser, pause=0.)


if __name__ == '__main__':
    unittest.main()
