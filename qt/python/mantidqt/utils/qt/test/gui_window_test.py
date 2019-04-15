# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import print_function
import inspect
import platform
import sys
from unittest import TestCase

from mantidqt.utils.qt.test.gui_test_runner import open_in_window
from qtpy import PYQT_VERSION
from qtpy.QtWidgets import QPushButton, QMenu, QAction, QApplication, QWidget
from qtpy.QtGui import QMouseEvent
from qtpy.QtCore import Qt, QMetaObject, QTime, QEvent
from qtpy.QtTest import QTest


def on_ubuntu_or_darwin():
    return ('Ubuntu' in platform.platform() and sys.version[0] == '2' or
            sys.platform == 'darwin' and PYQT_VERSION[0] == '4')


def trigger_action(action):
    QMetaObject.invokeMethod(action, 'trigger', Qt.QueuedConnection)


def find_action_with_text(widget, text):
    for action in widget.findChildren(QAction):
        if action.text() == text:
            return action
    raise RuntimeError("Couldn't find action with text \"{}\"".format(text))


def print_tree(widget, indent=0):
    if indent == 0:
        print('Children widgets of ', widget.objectName(), type(widget))
    space = ' ' * indent
    for w in widget.children():
        if isinstance(w, QWidget):
            text = '({})'.format(w.text()) if hasattr(w, 'text') else ''
            print('{}{} {} {}'.format(space, w, w.objectName(), text))
            print_tree(w, indent + 4)


def discover_children(widget, child_type=QWidget):
    print('Children widgets of ', widget.objectName(), type(widget))
    for w in widget.findChildren(child_type):
        text = '({})'.format(w.text()) if hasattr(w, 'text') else ''
        print(w, w.objectName(), text)


def get_child(widget, object_name, child_type=QWidget, timeout=3):
    t = QTime()
    t.start()
    timeout *= 1000
    children = []
    while len(children) == 0 and t.elapsed() < timeout:
        children = widget.findChildren(child_type, object_name)
        # QApplication.processEvents()
    if len(children) == 0:
        raise RuntimeError("Widget doesn't have child with name {}".format(object_name))
    if len(children) > 1:
        print('Widget has more than 1 child with name {}'.format(object_name))
    return children[0]


def drag_mouse(widget, from_pos, to_pos):
    QTest.mousePress(widget, Qt.LeftButton, Qt.NoModifier, from_pos)
    yield
    QTest.mouseMove(widget, from_pos)
    yield 0.1
    QTest.mouseMove(widget, to_pos)
    yield 0.1
    QTest.mouseRelease(widget, Qt.LeftButton, Qt.NoModifier, to_pos)
    yield 0.1


class GuiTestBase(object):

    def _call_test_method(self, w):
        self.widget = w
        if hasattr(self, self.call_method):
            return getattr(self, self.call_method)()
        if self.call_method != 'call':
            raise RuntimeError("Test class has no method {}".format(self.call_method))

    def wait_for(self, var_name):
        var = getattr(self, var_name)
        while var is None:
            yield
            var = getattr(self, var_name)

    def wait_for_true(self, fun, timeout_sec=1.0):
        t = QTime()
        t.start()
        timeout_millisec = int(timeout_sec * 1000)
        while not fun():
            if t.elapsed() > timeout_millisec:
                raise StopIteration()
            yield

    def wait_for_modal(self):
        return self.wait_for_true(self.get_active_modal_widget)

    def wait_for_popup(self):
        return self.wait_for_true(self.get_active_popup_widget)

    def run_test(self, method='call', pause=0, close_on_finish=True, attach_debugger=False):
        self.call_method = method
        open_in_window(self.create_widget, self._call_test_method, attach_debugger=attach_debugger, pause=pause,
                       close_on_finish=close_on_finish)

    def get_child(self, child_class, name):
        children = self.widget.findChildren(child_class, name)
        if len(children) == 0:
            raise RuntimeError("Widget doesn't have children of type {0} with name {1}.".format(child_class.__name__,
                                                                                                name))
        return children[0]

    @staticmethod
    def get_active_modal_widget():
        return QApplication.activeModalWidget()

    @staticmethod
    def get_active_popup_widget():
        return QApplication.activePopupWidget()

    def get_menu(self, name):
        return self.get_child(QMenu, name)

    def get_action(self, name, get_menu=False):
        action = self.get_child(QAction, name)
        if not get_menu:
            return action
        menus = action.associatedWidgets()
        if len(menus) == 0:
            raise RuntimeError("QAction {} isn't associated with any menu".format(name))
        return action, menus[0]

    def trigger_action(self, name):
        action, menu = self.get_action(name, get_menu=True)
        if not menu.isVisible():
            raise RuntimeError("Action {} isn't visible.".format(name))
        QMetaObject.invokeMethod(action, 'trigger', Qt.QueuedConnection)
        menu.close()

    def hover_action(self, name):
        action, menu = self.get_action(name, get_menu=True)
        if not menu.isVisible():
            raise RuntimeError("Action {} isn't visible.".format(name))
        QMetaObject.invokeMethod(action, 'hover', Qt.QueuedConnection)

    def get_button(self, name):
        return self.get_child(QPushButton, name)

    def click_button(self, name):
        button = self.get_button(name)
        QMetaObject.invokeMethod(button, 'click', Qt.QueuedConnection)

    def show_context_menu(self, widget, pos, pause=0):
        QTest.mouseMove(widget, pos)
        yield pause
        QTest.mouseClick(widget, Qt.RightButton, Qt.NoModifier, pos)
        yield pause
        ev = QMouseEvent(QEvent.ContextMenu, pos, Qt.RightButton, Qt.NoButton, Qt.NoModifier)
        QApplication.postEvent(widget, ev)
        yield self.wait_for_popup()

    def mouse_trigger_action(self, name, pause=0):
        menu = self.get_active_popup_widget()
        a, m = self.get_action(name, get_menu=True)
        pos = menu.actionGeometry(a).center()
        QTest.mouseMove(menu, pos)
        yield pause
        self.hover_action(name)
        QTest.mousePress(menu, Qt.LeftButton, Qt.NoModifier, pos)
        yield pause
        QTest.mouseRelease(menu, Qt.LeftButton, Qt.NoModifier, pos)


def is_test_method(value):
    if not (inspect.ismethod(value) or inspect.isfunction(value)):
        return False
    return value.__name__.startswith('test_')


class GuiWindowTest(TestCase, GuiTestBase):

    @classmethod
    def make_test_wrapper(cls, wrapped_name):
        def wrapper(self):
            self.run_test(method=wrapped_name)
        return wrapper

    @classmethod
    def setUpClass(cls):
        cls.test_methods = []
        cls.widget = None
        for test in inspect.getmembers(cls, is_test_method):
            name = test[0]
            wrapped_name = '_' + name
            setattr(cls, wrapped_name, test[1])
            setattr(cls, name, cls.make_test_wrapper(wrapped_name))


class MultiTestRunner(object):

    def __init__(self, methods):
        self.methods = methods

    def __call__(self, w):
        for method in self.methods:
            yield method


class WorkbenchGuiTest(GuiWindowTest):

    @classmethod
    def make_test_wrapper(cls, wrapped_name):
        def wrapper(self):
            if len(self.test_methods) == 0:
                self.widget = self.create_widget()
            self.test_methods.append(getattr(self, wrapped_name))
        return wrapper

    @classmethod
    def tearDownClass(cls):
        runner = MultiTestRunner(cls.test_methods)
        open_in_window(cls.widget, runner, close_on_finish=True, attach_debugger=False, in_workbench=True)

    def create_widget(self):
        qapp = QApplication.instance()
        for w in qapp.allWidgets():
            if w.objectName() == "Mantid Workbench":
                return w
        return qapp.activeWindow()
