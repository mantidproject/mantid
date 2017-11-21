import sys
from PyQt4.QtGui import QApplication
from PyQt4.QtCore import Qt, QTimer


class Tester(object):

    def __init__(self, widget):
        self.widget = widget
        self.input_text = None
        self.timer = None

    def __call__(self):
        widget = self.widget
        items = widget.tree.findItems('Squares v.1', Qt.MatchExactly | Qt.MatchRecursive)
        print items
        widget.tree.setCurrentItem(items[0])
        print widget.get_selected_algorithm()

        # self.widget.close()

    def idle(self):
        modal_widget = QApplication.activeModalWidget()
        if modal_widget is not None:
            modal_widget.setTextValue('Hello')
            modal_widget.accept()

    def start(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.idle)
        self.timer.start()
        QTimer.singleShot(0, self)


def import_widget(widget_path):
    """
    Imports a widget from a module in mantidqt
    :param widget_path: A qualified name of a widget, ie mantidqt.mywidget.MyWidget
    :return: The widget's class.
    """
    parts = widget_path.split('.')
    if len(parts) < 2:
        raise RuntimeError('Widget name must include name of the module in which it is defined')
    module_name = '.'.join(parts[:-1])
    widget_name = parts[-1]
    m = __import__(module_name, fromlist=[widget_name])
    return getattr(m, widget_name)


def open_in_window(widget_name):
    """
    Displays a widget in a window.
    :param widget_name:  A qualified name of a widget, ie mantidqt.mywidget.MyWidget
    """
    app = QApplication([""])

    w = import_widget(widget_name)()
    w.setWindowTitle(widget_name)
    w.show()

    # tester = Tester(w)
    # tester.start()

    sys.exit(app.exec_())


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("widget", help="A qualified name of a widget to open for testing. The name must contain the "
                                       "python module where the widget is defined, ie mypackage.mymodule.MyWidget")
    args = parser.parse_args()
    open_in_window(args.widget)
