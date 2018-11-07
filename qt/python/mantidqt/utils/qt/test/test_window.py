from __future__ import print_function

from mantidqt.utils.qt.test.run_test_app import open_in_window
from qtpy.QtWidgets import QPushButton, QMenu, QAction
from qtpy.QtCore import QObject, Signal, Qt


class modals(object):
    def __init__(self, *methods):
        self.methods = methods

    def _make_caller(self, method):
        def caller(self):
            self.start.emit(method)
        return caller

    def __call__(self, cls):

        for method in self.methods:
            slot = getattr(cls, method)
            method_slot = '{}_slot'.format(method)
            setattr(cls, method, self._make_caller(method_slot))
            setattr(cls, method_slot, slot)

        return cls


class GuiTestBase(QObject, object):

    start = Signal('QString')

    def slot(self, method):
        getattr(self, method)()

    def wait_for(self, var_name):
        var = getattr(self, var_name)
        while var is None:
            yield

    def __init__(self):
        super(GuiTestBase, self).__init__()
        self.start.connect(self.slot, type=Qt.QueuedConnection)
        self.widget = None

    def __call__(self, w):
        self.widget = w
        return self.call()

    @classmethod
    def run(cls, window_class, pause=0, close_on_finish=True):
        open_in_window(window_class, cls, attach_debugger=False, pause=pause, close_on_finish=close_on_finish)

    def get_child(self, child_class, name):
        children = self.widget.findChildren(child_class, name)
        if len(children) == 0:
            raise RuntimeError("Widget doesn't have children of type {0} with name {1}.".format(child_class.__name__,
                                                                                                name))
        return children[0]

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
        action.trigger()
        menu.close()

    def get_button(self, name):
        return self.get_child(QPushButton, name)
