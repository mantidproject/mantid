from __future__ import print_function

from mantidqt.utils.qt.test.run_test_app import open_in_window
from qtpy.QtWidgets import QPushButton, QMenu, QAction
from qtpy.QtCore import QObject, Signal, Qt, QMetaObject


class modals(object):
    def __init__(self, *methods):
        self.methods = methods

    @staticmethod
    def _make_caller(method_name):
        def caller(self):
            setattr(self, modals._make_finished_name(method_name), False)
            self.start.emit(method_name)
        return caller

    @staticmethod
    def _make_finished_checker(method_name):
        finished_name = modals._make_finished_name(method_name)

        def checker(self):
            var = getattr(self, finished_name)
            while not var:
                yield
                var = getattr(self, finished_name)
        return checker

    @staticmethod
    def _make_slot_name(method):
        return '_{}_slot___'.format(method)

    @staticmethod
    def _make_slot(method_name, method):
        def slot(self):
            method(self)
            print('FINISHING', modals._make_finished_name(method_name))
            setattr(self, modals._make_finished_name(method_name), True)
            return getattr(self, modals.make_finished_checker_name(method_name))()
        return slot

    @staticmethod
    def _make_finished_name(method):
        return '_{}_finished___'.format(method)

    @staticmethod
    def make_finished_checker_name(method):
        return '_is_{}_finished___'.format(method)

    def __call__(self, cls):

        cls._modal_methods = self.methods
        for method_name in self.methods:
            method = getattr(cls, method_name)
            method_slot_name = self._make_slot_name(method_name)
            setattr(cls, method_name, self._make_caller(method_slot_name))
            setattr(cls, method_slot_name, self._make_slot(method_name, method))
            setattr(cls, self._make_finished_name(method_name), False)
            setattr(cls, self.make_finished_checker_name(method_name), self._make_finished_checker(method_name))

        if not hasattr(cls, 'call'):
            def call(self):
                pass
            setattr(cls, 'call', call)

        return cls


class GuiTestBase(QObject, object):

    start = Signal('QString')

    def __init__(self):
        super(GuiTestBase, self).__init__()
        self.start.connect(self.modal_slot_caller, type=Qt.QueuedConnection)
        self.widget = None
        self.call_method = 'call'

    def __call__(self, w):
        self.widget = w
        return getattr(self, self.call_method)()

    def modal_slot_caller(self, method):
        getattr(self, method)()

    def wait_for(self, var_name):
        var = getattr(self, var_name)
        while var is None:
            yield
            var = getattr(self, var_name)

    def wait_for_true(self, var_name):
        var = getattr(self, var_name)
        while not var:
            yield
            var = getattr(self, var_name)

    def wait_to_finish(self, method_name):
        if method_name not in self._modal_methods:
            raise RuntimeError("'{}' is not one of the modal methods".format(method_name))
        return getattr(self, modals.make_finished_checker_name(method_name))()

    def run(self, window_class, method='call', pause=0, close_on_finish=True):
        self.call_method = method
        open_in_window(window_class, self, attach_debugger=False, pause=pause, close_on_finish=close_on_finish)

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
