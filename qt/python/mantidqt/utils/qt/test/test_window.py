from __future__ import print_function

from mantidqt.utils.qt.test.run_test_app import open_in_window
from qtpy.QtWidgets import QPushButton, QMenu, QAction, QApplication
from qtpy.QtCore import QObject, Signal, Qt, QMetaObject


def _make_caller(method_name):
    def caller(self):
        # setattr(self, _make_finished_name(method_name), False)
        self.start.emit(method_name)

    return caller


def _make_finished_checker(method_name):
    finished_name = _make_finished_name(method_name)

    def checker(self):
        var = getattr(self, finished_name)
        while not var:
            yield
            var = getattr(self, finished_name)

    return checker


def _make_slot_name(method):
    return '_{}_slot___'.format(method)


def _make_slot(method_name, method):
    def slot(self):
        method(self)
        setattr(self, _make_finished_name(method_name), True)
        return getattr(self, make_finished_checker_name(method_name))()

    return slot


def _make_finished_name(method):
    return '_{}_finished___'.format(method)


def make_finished_checker_name(method):
    return '_is_{}_finished___'.format(method)


class modals(object):
    def __init__(self, *methods):
        self.methods = methods

    def __call__(self, cls):

        cls._modal_methods = self.methods
        for method_name in self.methods:
            method = getattr(cls, method_name)
            method_slot_name = _make_slot_name(method_name)
            setattr(cls, method_name, _make_caller(method_slot_name))
            setattr(cls, method_slot_name, _make_slot(method_name, method))
            setattr(cls, _make_finished_name(method_name), False)
            setattr(cls, make_finished_checker_name(method_name), _make_finished_checker(method_name))

        return cls


def make_modals(cls):

    if not hasattr(cls, '_modal_methods'):
        cls._modal_methods = []
    cls_vars = [name for name in vars(cls)]
    for method_name in cls_vars:
        caller = getattr(cls, method_name)
        if hasattr(caller, 'method'):
            method = caller.method
            setattr(cls, caller.method_slot_name, _make_slot(method_name, method))
            setattr(cls, _make_finished_name(method_name), False)
            setattr(cls, make_finished_checker_name(method_name), _make_finished_checker(method_name))
            cls._modal_methods.append(method_name)
    return cls


def modal(method):

    method_name = method.__name__
    method_slot_name = _make_slot_name(method_name)
    caller = _make_caller(method_slot_name)
    caller.method = method
    caller.method_name = method_name
    caller.method_slot_name = method_slot_name
    return caller


class GuiTestBase(QObject, object):

    start = Signal('QString')

    def __init__(self):
        super(GuiTestBase, self).__init__()
        self.start.connect(self.modal_slot_caller, type=Qt.QueuedConnection)
        self.widget = None
        self.call_method = 'call'

    def __call__(self, w):
        self.widget = w
        if hasattr(self, self.call_method):
            return getattr(self, self.call_method)()
        if self.call_method != 'call':
            raise RuntimeError("Test class has no method {}".format(self.call_method))

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

    def get_active_modal_widget(self):
        return QApplication.activeModalWidget()

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
