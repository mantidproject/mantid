from __future__ import (absolute_import, division, print_function)
import unittest

from qtpy.QtWidgets import QApplication, QWidget, QAction
from qtpy.QtCore import Qt, QPoint, QMetaObject, QEvent
from qtpy.QtTest import QTest

from mantid import FrameworkManager, FunctionFactory
from mantid.fitfunctions import FunctionWrapper
from mantidqt.utils.qt.test.gui_window_test import (GuiWindowTest, on_ubuntu_or_darwin, print_tree, discover_children,
                                                    get_child)
from mantidqt.utils.qt import import_qt
from mantidqt.utils.qt.test import GuiTest


FunctionBrowser = import_qt('.._common', 'mantidqt.widgets', 'FunctionBrowser')
skip = unittest.skipIf(on_ubuntu_or_darwin(),
                       "Popups don't show on ubuntu with python 2. Unskip when switched to xvfb."
                       "Qt4 has a bug on macs which is fixed in Qt5.")


class TestFunctionBrowser(GuiWindowTest):

    is_multi = False

    def create_widget(self):
        return FunctionBrowser(None, self.is_multi)

    def view_set_parameter(self, param_name, value):
        view = self.widget.view()
        rect = view.getVisualRectParameterProperty(param_name)
        pos = rect.center()
        if self.is_multi:
            pos -= QPoint(rect.width() / 5, 0)
        else:
            pos += QPoint(rect.width() / 4, 0)
        tree = view.treeWidget().viewport()
        QTest.mouseMove(tree, pos)
        yield
        QTest.mouseClick(tree, Qt.LeftButton, Qt.NoModifier, pos)
        yield
        editor = QApplication.focusWidget()
        QTest.keyClicks(editor, str(value))
        QTest.keyClick(editor, Qt.Key_Return)

    def get_fit_function(self):
        fun_str = self.widget.getFitFunctionString()
        return FunctionWrapper.wrap(FunctionFactory.createInitialized(fun_str))


def function_browser_test(multi=False):

    def wrapper(fun):
        cls = type(fun.__name__, (TestFunctionBrowser,), dict(test=fun, is_multi = multi))
        return skip(cls)

    return wrapper


def dummy_test(multi=False):
    return lambda x: None


function_browser_test_ = function_browser_test
function_browser_test = dummy_test


@function_browser_test(False)
def test_browser_parameters_view(self):
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    # discover_children(view, QAction)
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertGreater(len(browser.getFunctionString()), 0)
    pos = view.getVisualRectFunctionProperty('f1.').center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action('remove_function', pause=0)
    self.assertEqual(view.getNumberOfQtProperties(), 2)


@function_browser_test(False)
def test_browser_fun_parameters(self):
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertGreater(len(browser.getFunctionString()), 0)
    self.assertAlmostEquals(browser.getParameter('f0.A0'), 0.0)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 1.0)
    browser.setParameter('f0.A0', 2.2)
    browser.setParameter('f1.A0', 3.3)
    self.assertAlmostEquals(browser.getParameter('f0.A0'), 2.2)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 3.3)
    self.assertAlmostEquals(view.getParameter('f0.A0'), 2.2)
    self.assertAlmostEquals(view.getParameter('f1.A0'), 3.3)
    yield self.view_set_parameter('f1.A0', 5.5)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 5.5)
    self.assertAlmostEquals(view.getParameter('f1.A0'), 5.5)
    # yield 100


@function_browser_test(True)
def test_browser_parameters_multi_view(self):
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertGreater(len(browser.getFunctionString()), 0)
    self.assertEqual(browser.getNumberOfDatasets(), 1)
    pos = view.getVisualRectFunctionProperty('f1.').center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action('remove_function', pause=0)
    self.assertEqual(view.getNumberOfQtProperties(), 2)
    yield 0.1


@function_browser_test(True)
def test_browser_parameters_multi_single(self):
    assert(isinstance(self, TestFunctionBrowser))
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertEqual(browser.getNumberOfDatasets(), 1)
    self.assertGreater(len(browser.getFunctionString()), 0)
    self.assertAlmostEquals(browser.getParameter('f0.A0'), 0.0)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 1.0)
    browser.setParameter('f0.A0', 2.2)
    browser.setParameter('f1.A0', 3.3)
    self.assertAlmostEquals(browser.getParameter('f0.A0'), 2.2)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 3.3)
    self.assertAlmostEquals(view.getParameter('f0.A0'), 2.2)
    self.assertAlmostEquals(view.getParameter('f1.A0'), 3.3)
    yield self.view_set_parameter('f1.A0', 5.5)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 5.5)
    self.assertAlmostEquals(view.getParameter('f1.A0'), 5.5)
    yield 0.1


@function_browser_test(True)
def test_browser_parameters_multi_multi(self):
    assert(isinstance(self, TestFunctionBrowser))
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    browser.setNumberOfDatasets(3)
    self.assertEqual(browser.getNumberOfDatasets(), 3)
    fun_str = browser.getFitFunctionString()
    self.assertGreater(len(fun_str), 0)
    self.assertAlmostEquals(browser.getParameter('f0.A0'), 0.0)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 1.0)
    browser.setParameter('f0.A0', 2.2)
    browser.setParameter('f1.A0', 3.3)
    self.assertAlmostEquals(browser.getParameter('f0.A0'), 2.2)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 3.3)
    self.assertAlmostEquals(view.getParameter('f0.A0'), 2.2)
    self.assertAlmostEquals(view.getParameter('f1.A0'), 3.3)
    yield self.view_set_parameter('f1.A0', 5.5)
    self.assertAlmostEquals(browser.getParameter('f1.A0'), 5.5)
    self.assertAlmostEquals(view.getParameter('f1.A0'), 5.5)
    yield 0.1


@function_browser_test(True)
def test_fit_function_multi(self):
    assert(isinstance(self, TestFunctionBrowser))
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    browser.setNumberOfDatasets(3)
    self.assertEqual(browser.getNumberOfDatasets(), 3)
    fun = self.get_fit_function()
    self.assertEqual(fun.name, 'MultiDomainFunction')
    self.assertEqual(fun.nDomains, 3)
    self.assertEqual(fun[0][0].A0, 0.0)
    self.assertEqual(fun[0][1].A0, 1.0)
    self.assertEqual(fun[1][0].A0, 0.0)
    self.assertEqual(fun[1][1].A0, 1.0)
    self.assertEqual(fun[2][0].A0, 0.0)
    self.assertEqual(fun[2][1].A0, 1.0)


@function_browser_test(True)
def test_multi_set_parameters_different_domains(self):
    assert(isinstance(self, TestFunctionBrowser))
    browser = self.widget
    self.widget.setFunction('name=FlatBackground;name=FlatBackground,A0=1')
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    browser.setNumberOfDatasets(3)
    self.assertEqual(browser.getNumberOfDatasets(), 3)
    self.assertEqual(browser.getCurrentDataset(), 0)
    browser.setCurrentDataset(1)
    self.assertEqual(browser.getCurrentDataset(), 1)
    browser.setParameter('f0.A0', 2.2)
    browser.setParameter('f1.A0', 3.3)
    browser.setCurrentDataset(2)
    self.assertEqual(browser.getCurrentDataset(), 2)
    browser.setParameter('f0.A0', 4.4)
    browser.setParameter('f1.A0', 5.5)
    fun = self.get_fit_function()
    self.assertEqual(fun[0][0].A0, 0.0)
    self.assertEqual(fun[0][1].A0, 1.0)
    self.assertEqual(fun[1][0].A0, 2.2)
    self.assertEqual(fun[1][1].A0, 3.3)
    self.assertEqual(fun[2][0].A0, 4.4)
    self.assertEqual(fun[2][1].A0, 5.5)
    yield 100


if __name__ == '__main__':
    unittest.main()
    FrameworkManager.clear()
