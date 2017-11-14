import sys
from qtpy.QtWidgets import QApplication


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


def main(widget_name):
    """
    Displays a widget in a window.
    :param widget_name:  A qualified name of a widget, ie mantidqt.mywidget.MyWidget
    """
    app = QApplication([""])

    w = import_widget(widget_name)()
    w.setWindowTitle(widget_name)
    w.show()

    sys.exit(app.exec_())


if __name__ == "__main__":
    if len(sys.argv) > 1:
        widget_name = sys.argv[1]
        main(widget_name)
    else:
        print('Give a name of a widget to display')
