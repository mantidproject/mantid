# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from qtpy.QtWidgets import QApplication
from qtpy.QtCore import Qt, QPoint, QObject
from qtpy.QtTest import QTest

from mantid import FrameworkManager, FunctionFactory
from mantid.fitfunctions import FunctionWrapper
from unittest.mock import MagicMock, call
from mantidqt.utils.qt.testing.gui_window_test import GuiWindowTest, not_on_windows, get_child, click_on
from mantidqt.widgets.functionbrowser import FunctionBrowser

skip = unittest.skipIf(not_on_windows(), "It works on windows. I cannot spend too much time trying to " "fix the other platforms.")


class TestFunctionBrowser(GuiWindowTest):
    is_multi = False

    def create_widget(self):
        widget = FunctionBrowser(None, self.is_multi)
        # widget.resize(500, 500)  # The tests will fail when run on a high-resolution screen locally
        return widget

    def view_set_parameter(self, param_name, value):
        view = self.widget.view()
        rect = view.getVisualRectParameterProperty(param_name)
        pos = rect.center()
        if self.is_multi:
            pos -= QPoint(int(rect.width() / 5), 0)
        else:
            pos += QPoint(int(rect.width() / 4), 0)
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
        cls = type(fun.__name__, (TestFunctionBrowser,), dict(test=fun, is_multi=multi))
        return skip(cls)

    return wrapper


def dummy_test(multi=False):
    return lambda x: None


function_browser_test_ = function_browser_test


# function_browser_test = dummy_test


class BrowserUser(QObject):
    def __init__(self, browser):
        super(BrowserUser, self).__init__()
        self.structure_changed = MagicMock()
        self.parameter_changed = MagicMock()
        browser.functionStructureChanged.connect(self.structure_changed)
        browser.parameterChanged.connect(self.parameter_changed)


@function_browser_test(False)
def test_browser_parameters_view(self):
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertGreater(len(browser.getFunctionString()), 0)
    pos = view.getVisualRectFunctionProperty("f1.").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action("remove_function", pause=0)
    self.assertEqual(view.getNumberOfQtProperties(), 2)


@function_browser_test(False)
def test_browser_fun_parameters(self):
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertGreater(len(browser.getFunctionString()), 0)
    self.assertAlmostEqual(browser.getParameter("f0.A0"), 0.0)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 1.0)
    browser.setParameter("f0.A0", 2.2)
    browser.setParameter("f1.A0", 3.3)
    self.assertAlmostEqual(browser.getParameter("f0.A0"), 2.2)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 3.3)
    self.assertAlmostEqual(view.getParameter("f0.A0"), 2.2)
    self.assertAlmostEqual(view.getParameter("f1.A0"), 3.3)
    yield self.view_set_parameter("f1.A0", 5.5)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 5.5)
    self.assertAlmostEqual(view.getParameter("f1.A0"), 5.5)
    # yield 100


@function_browser_test(True)
def test_browser_parameters_multi_view(self):
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertGreater(len(browser.getFunctionString()), 0)
    self.assertEqual(browser.getNumberOfDatasets(), 0)
    pos = view.getVisualRectFunctionProperty("f1.").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=2)
    yield self.mouse_trigger_action("remove_function", pause=0)
    self.assertEqual(view.getNumberOfQtProperties(), 2)
    yield 0.1


@function_browser_test(True)
def test_browser_parameters_multi_single(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    self.assertEqual(browser.getNumberOfDatasets(), 0)
    self.assertGreater(len(browser.getFunctionString()), 0)
    self.assertAlmostEqual(browser.getParameter("f0.A0"), 0.0)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 1.0)
    browser.setParameter("f0.A0", 2.2)
    browser.setParameter("f1.A0", 3.3)
    self.assertAlmostEqual(browser.getParameter("f0.A0"), 2.2)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 3.3)
    self.assertAlmostEqual(view.getParameter("f0.A0"), 2.2)
    self.assertAlmostEqual(view.getParameter("f1.A0"), 3.3)
    yield self.view_set_parameter("f1.A0", 5.5)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 5.5)
    self.assertAlmostEqual(view.getParameter("f1.A0"), 5.5)
    yield 0.1


@function_browser_test(True)
def test_browser_parameters_multi_multi(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=LinearBackground,A0=1")
    view = browser.view()
    user = BrowserUser(browser)
    self.assertEqual(view.getNumberOfQtProperties(), 9)
    browser.setNumberOfDatasets(3)
    self.assertEqual(browser.getNumberOfDatasets(), 3)
    fun_str = browser.getFitFunctionString()
    self.assertGreater(len(fun_str), 0)
    self.assertAlmostEqual(browser.getParameter("f0.A0"), 0.0)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 1.0)
    browser.setParameter("f0.A0", 2.2)
    browser.setParameter("f1.A0", 3.3)
    self.assertAlmostEqual(browser.getParameter("f0.A0"), 2.2)
    self.assertAlmostEqual(browser.getParameter("f1.A0"), 3.3)
    self.assertAlmostEqual(view.getParameter("f0.A0"), 2.2)
    self.assertAlmostEqual(view.getParameter("f1.A0"), 3.3)
    yield self.view_set_parameter("f1.A1", 5.5)
    self.assertAlmostEqual(browser.getParameter("f1.A1"), 5.5)
    self.assertAlmostEqual(view.getParameter("f1.A1"), 5.5)
    self.assertGreater(user.parameter_changed.call_count, 0)
    self.assertEqual(user.parameter_changed.call_args_list[-1], call("f1.", "A1"))
    yield 0.1


@function_browser_test(True)
def test_fit_function_multi(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    browser.setNumberOfDatasets(3)
    self.assertEqual(browser.getNumberOfDatasets(), 3)
    fun = self.get_fit_function()
    self.assertEqual(fun.name, "MultiDomainFunction")
    self.assertEqual(fun.nDomains, 3)
    self.assertEqual(fun[0][0].A0, 0.0)
    self.assertEqual(fun[0][1].A0, 1.0)
    self.assertEqual(fun[1][0].A0, 0.0)
    self.assertEqual(fun[1][1].A0, 1.0)
    self.assertEqual(fun[2][0].A0, 0.0)
    self.assertEqual(fun[2][1].A0, 1.0)


@function_browser_test(True)
def test_multi_set_parameters_different_domains(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    self.assertEqual(view.getNumberOfQtProperties(), 8)
    browser.setNumberOfDatasets(3)
    self.assertEqual(browser.getNumberOfDatasets(), 3)
    self.assertEqual(browser.getCurrentDataset(), 0)
    browser.setCurrentDataset(1)
    self.assertEqual(browser.getCurrentDataset(), 1)
    browser.setParameter("f0.A0", 2.2)
    browser.setParameter("f1.A0", 3.3)
    browser.setCurrentDataset(2)
    self.assertEqual(browser.getCurrentDataset(), 2)
    browser.setParameter("f0.A0", 4.4)
    browser.setParameter("f1.A0", 5.5)
    fun = self.get_fit_function()
    self.assertEqual(fun[0][0].A0, 0.0)
    self.assertEqual(fun[0][1].A0, 1.0)
    self.assertEqual(fun[1][0].A0, 2.2)
    self.assertEqual(fun[1][1].A0, 3.3)
    self.assertEqual(fun[2][0].A0, 4.4)
    self.assertEqual(fun[2][1].A0, 5.5)


@function_browser_test()
def test_paste_from_clipboard(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=FlatBackground,A0=1")
    view = browser.view()
    user = BrowserUser(browser)

    QApplication.clipboard().setText("name=LinearBackground,A0=5,A1=10")
    pos = view.getVisualRectFunctionProperty("").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action("paste_from_clipboard", pause=0)
    fun = self.get_fit_function()
    self.assertEqual(fun.name, "LinearBackground")
    self.assertEqual(user.structure_changed.call_count, 2)


@function_browser_test()
def test_add_function(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    view = browser.view()
    user = BrowserUser(browser)
    pos = view.getVisualRectFunctionProperty("").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action("add_function", pause=0)
    yield self.wait_for_modal()
    dlg = self.get_active_modal_widget()
    tree = get_child(dlg, "fitTree")
    item = tree.findItems("Background", Qt.MatchExactly)[0]
    item.setExpanded(True)
    item = tree.findItems("FlatBackground", Qt.MatchRecursive)[0]
    pos = tree.visualItemRect(item).center()
    yield click_on(tree.viewport(), pos)
    dlg.accept()
    fun = self.get_fit_function()
    self.assertEqual(fun.name, "FlatBackground")
    self.assertEqual(user.structure_changed.call_count, 2)


@function_browser_test()
def test_add_function_to_composite(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;(name=FlatBackground,A0=1;name=FlatBackground,A0=2)")
    view = browser.view()
    user = BrowserUser(browser)
    pos = view.getVisualRectFunctionProperty("f1.").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action("add_function", pause=0)
    yield self.wait_for_modal()
    dlg = self.get_active_modal_widget()
    tree = get_child(dlg, "fitTree")
    item = tree.findItems("Background", Qt.MatchExactly)[0]
    item.setExpanded(True)
    item = tree.findItems("LinearBackground", Qt.MatchRecursive)[0]
    pos = tree.visualItemRect(item).center()
    yield click_on(tree.viewport(), pos)
    dlg.accept()
    fun = self.get_fit_function()
    self.assertEqual(fun.name, "CompositeFunction")
    self.assertEqual(fun[0].name, "FlatBackground")
    self.assertEqual(fun[1].name, "CompositeFunction")
    self.assertEqual(fun[1][0].name, "FlatBackground")
    self.assertEqual(fun[1][1].name, "FlatBackground")
    self.assertEqual(fun[1][2].name, "LinearBackground")
    self.assertEqual(fun[1][0].A0, 1.0)
    self.assertEqual(fun[1][1].A0, 2.0)
    self.assertEqual(fun[1][2].A0, 0.0)
    self.assertEqual(user.structure_changed.call_count, 2)


@function_browser_test(multi=True)
def test_converting_to_composite_retains_globals(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground")
    browser.setGlobalParameters(["A0"])
    view = browser.view()
    pos = view.getVisualRectFunctionProperty("").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action("add_function", pause=0)
    yield self.wait_for_modal()
    dlg = self.get_active_modal_widget()
    tree = get_child(dlg, "fitTree")
    item = tree.findItems("Background", Qt.MatchExactly)[0]
    item.setExpanded(True)
    item = tree.findItems("BSpline", Qt.MatchRecursive)[0]
    pos = tree.visualItemRect(item).center()
    yield click_on(tree.viewport(), pos, pause=10)
    dlg.accept()
    self.assertEqual(browser.getGlobalParameters(), ["f0.A0"])


@function_browser_test(multi=True)
def test_converting_to_single_retains_globals(self):
    assert isinstance(self, TestFunctionBrowser)
    browser = self.widget
    browser.setFunction("name=FlatBackground;name=BSpline")
    browser.setGlobalParameters(["f0.A0"])
    view = browser.view()
    pos = view.getVisualRectFunctionProperty("f1.").center()
    tree = view.treeWidget().viewport()
    yield self.show_context_menu(tree, pos, pause=0)
    yield self.mouse_trigger_action("remove_function", pause=0)
    self.assertEqual(browser.getGlobalParameters(), ["A0"])


if __name__ == "__main__":
    unittest.main()
    FrameworkManager.clear()
