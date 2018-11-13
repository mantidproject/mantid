from __future__ import print_function

from mantidqt.utils.qt.test.run_test_app import open_in_window
from qtpy.QtWidgets import QPushButton, QMenu, QAction, QApplication
from qtpy.QtCore import QObject, Signal, Qt, QMetaObject


class GuiTestBase(QObject, object):

    def __init__(self):
        super(GuiTestBase, self).__init__()
        self.widget = None
        self.call_method = 'call'

    def __call__(self, w):
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

    def wait_for_true(self, fun):
        while not fun():
            yield

    def wait_for_modal(self):
        return self.wait_for_true(self.get_active_modal_widget)

    def wait_for_popup(self):
        return self.wait_for_true(self.get_active_popup_widget)

    def run(self, window_class, method='call', pause=0, close_on_finish=True, attach_debugger=False):
        self.call_method = method
        open_in_window(window_class, self, attach_debugger=attach_debugger, pause=pause,
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
