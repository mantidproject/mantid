import sys
from qtpy.QtWidgets import QApplication, QPushButton
from qtpy.QtCore import Qt, QTimer
from qtpy.QtTest import QTest


class Worker(object):

    def __init__(self, widget):
        self.widget = widget

    def __call__(self):
        print self.widget
        button = self.widget.findChild(QPushButton, "The Button")
        print button.pos()
        # time.sleep(3)
        QTest.mouseMove(self.widget)
        QTest.mouseClick(button, Qt.LeftButton)


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

    # Worker(w)()
    # QTimer.singleShot(0, Worker(w))

    sys.exit(app.exec_())


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("widget", help="A qualified name of a widget to open for testing. The name must contain the "
                                       "python module where the widget is defined, ie mypackage.mymodule.MyWidget")
    args = parser.parse_args()
    open_in_window(args.widget)
