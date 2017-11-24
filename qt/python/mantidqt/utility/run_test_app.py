from __future__ import absolute_import, print_function
import sys
import ast
import traceback
from PyQt4.QtGui import QApplication, QContextMenuEvent, QAction
from PyQt4.QtCore import Qt, QTimer, QPoint


class Tester(object):

    def __init__(self, widget):
        self.widget = widget
        self.input_text = None
        self.timer = None

    def __call__(self):
        widget = self.widget
        items = widget.tree.findItems('Squares v.1', Qt.MatchExactly | Qt.MatchRecursive)
        print(items)
        widget.tree.setCurrentItem(items[0])
        print(widget.get_selected_algorithm())
        event = QContextMenuEvent(QContextMenuEvent.Mouse, QPoint())
        QApplication.postEvent(widget.search_box, event)

        # self.widget.close()

    def idle(self):
        modal_widget = QApplication.activeModalWidget()
        if modal_widget is not None:
            modal_widget.setTextValue('Hello')
            modal_widget.accept()
        pop_up = QApplication.activePopupWidget()
        if pop_up is not None:
            print(pop_up)
            for action in pop_up.children():
                if isinstance(action, QAction):
                    print(action.text())
                    if action.text() == '&Paste	Ctrl+V':
                        action.activate(QAction.Trigger)
            pop_up.close()

    def start(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.idle)
        self.timer.start()
        QTimer.singleShot(0, self)


def monitor_modals():
    modal_widget = QApplication.activeModalWidget()
    if modal_widget is not None:
        print(modal_widget)
    pop_up = QApplication.activePopupWidget()
    if pop_up is not None:
        print(pop_up)


def split_qualified_name(qualified_name):
    parts = qualified_name.split('.')
    if len(parts) < 2:
        raise RuntimeError('Qualified name must include name of the module in which it is defined,'
                           ' found: {0}'.format(qualified_name))
    module_name = '.'.join(parts[:-1])
    name = parts[-1]
    return module_name, name


def create_widget(widget_path):
    """
    Imports a widget from a module in mantidqt
    :param widget_path: A qualified name of a widget, ie mantidqt.mywidget.MyWidget
    :return: The widget's class.
    """
    module_name, widget_name = split_qualified_name(widget_path)
    m = __import__(module_name, fromlist=[widget_name])
    widget_generator = getattr(m, widget_name)
    return widget_generator()


def open_in_window(widget_name, script):
    """
    Displays a widget in a window.
    :param widget_name:  A qualified name of a widget, ie mantidqt.mywidget.MyWidget
    """
    app = QApplication([""])

    w = create_widget(widget_name)
    w.setWindowTitle(widget_name)
    w.show()

    if script is not None:
        run_script(script, w)

    # tester = Tester(w)
    # tester.start()

    # timer = QTimer()
    # timer.timeout.connect(monitor_modals)
    # timer.start()

    sys.exit(app.exec_())


def run_script(script_name, widget):
    module_name, fun_name = split_qualified_name(script_name)
    m = __import__(module_name, fromlist=[fun_name])
    fun = getattr(m, fun_name)
    try:
        fun(widget)
    except:
        traceback.print_exc()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("widget", help="A qualified name of a widget to open for testing. The name must contain the "
                                       "python module where the widget is defined, eg mypackage.mymodule.MyWidget")
    parser.add_argument("--script", help="A qualified name of a python function to run to test the widget."
                                         " The function must take a single argument - the widget."
                                         " The name must contain the python module where the function is defined,"
                                         " eg somepackage.somemodule.test_my_widget")
    args = parser.parse_args()
    open_in_window(args.widget, args.script)
