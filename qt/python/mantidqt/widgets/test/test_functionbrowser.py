from __future__ import (absolute_import, division, print_function)
import unittest

from qtpy.QtWidgets import QApplication, QWidget, QAction
from qtpy.QtCore import Qt, QMetaObject, QEvent
from qtpy.QtTest import QTest

from mantid import FrameworkManager
from mantidqt.utils.qt.test.gui_window_test import (GuiWindowTest, on_ubuntu_or_darwin, print_tree, discover_children,
                                                    get_child)
from mantidqt.utils.qt import import_qt
from mantidqt.utils.qt.test import GuiTest


FunctionBrowser = import_qt('.._common', 'mantidqt.widgets', 'FunctionBrowser')


@unittest.skipIf(on_ubuntu_or_darwin(), "Popups don't show on ubuntu with python 2. Unskip when switched to xvfb."
                                        "Qt4 has a bug on macs which is fixed in Qt5.")
class TestFunctionBrowser(GuiWindowTest):

    def create_widget(self):
        return FunctionBrowser()

    def test_view(self):
        browser = self.widget
        self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
        view = browser.view()
        # discover_children(view, QAction)
        self.assertEqual(view.getNumberOfQtProperties(), 8)
        print(':', browser.getFunctionString())
        pos = view.getVisualRectFunctionProperty('f1.').center()
        tree = view.treeWidget().viewport()
        yield self.show_context_menu(tree, pos, pause=0)
        yield self.mouse_trigger_action('remove_function', pause=0)
        self.assertEqual(view.getNumberOfQtProperties(), 2)

        yield 100


if __name__ == '__main__':
    unittest.main()
    FrameworkManager.clear()
