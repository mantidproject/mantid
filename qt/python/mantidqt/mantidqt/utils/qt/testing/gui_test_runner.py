# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import inspect
import traceback

from qtpy.QtCore import QTimer, QMetaObject, Qt
from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt.testing.application import get_application


def split_qualified_name(qualified_name):
    parts = qualified_name.split(".")
    if len(parts) < 2:
        raise RuntimeError("Qualified name must include name of the module in which it is defined, found: {0}".format(qualified_name))
    module_name = ".".join(parts[:-1])
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


class ScriptRunner(object):
    """
    Runs a script that interacts with a widget (tests it).
    If the script is a python generator then after each iteration controls returns
    to the QApplication's event loop.
    Generator scripts can yield a positive number. It is treated as the number of seconds
    before the next iteration is called. During the wait time the event loop is running.
    """

    def __init__(self, script, widget=None, close_on_finish=True, pause=0, is_cli=False):
        """
        Initialise a runner.
        :param script: The script to run.
        :param widget: The widget to test.
        :param close_on_finish: If true close the widget after the script has finished.
        :param is_cli: If true the script is to be run from a command line tool. Exceptions are
            treated slightly differently in this case.
        """
        app = get_application()
        self.script = script
        self.widget = widget
        self.close_on_finish = close_on_finish
        self.pause = pause
        self.is_cli = is_cli
        self.error = None
        self.script_iter = [None]
        self.pause_timer = QTimer(app)
        self.pause_timer.setSingleShot(True)
        self.script_timer = QTimer(app)

    def run(self):
        ret = run_script(self.script, self.widget)
        if isinstance(ret, Exception):
            raise ret
        self.script_iter = [iter(ret) if inspect.isgenerator(ret) else None]
        if self.pause != 0:
            self.script_timer.setInterval(self.pause * 1000)
        # Zero-timeout timer runs script_runner() between Qt events
        self.script_timer.timeout.connect(self, Qt.QueuedConnection)
        QMetaObject.invokeMethod(self.script_timer, "start", Qt.QueuedConnection)

    def __call__(self):
        app = get_application()
        if not self.pause_timer.isActive():
            try:
                script_iter = self.script_iter[-1]
                if script_iter is None:
                    if self.close_on_finish:
                        app.closeAllWindows()
                        app.exit()
                    return
                # Run test script until the next 'yield'
                try:
                    ret = next(script_iter)
                except ValueError:
                    return
                while ret is not None:
                    if inspect.isgenerator(ret):
                        self.script_iter.append(ret)
                        ret = None
                    elif isinstance(ret, int) or isinstance(ret, float):
                        # Start non-blocking pause in seconds
                        self.pause_timer.start(int(ret * 1000))
                        ret = None
                    else:
                        ret = ret()
            except StopIteration:
                if len(self.script_iter) > 1:
                    self.script_iter.pop()
                else:
                    self.script_iter = [None]
                    self.script_timer.stop()
                    if self.close_on_finish:
                        app.closeAllWindows()
                        app.exit(0)
            except Exception as e:
                self.script_iter = [None]
                traceback.print_exc()
                if self.close_on_finish:
                    app.exit(1)
                self.error = e


def open_in_window(widget_or_name, script, attach_debugger=True, pause=0, close_on_finish=False, is_cli=False, in_workbench=False):
    """
    Displays a widget in a window.
    :param widget_or_name: A widget to display.
            1. If a string it's a qualified name of a widget, eg mantidqt.mywidget.MyWidget
            2. If a callable it must take no arguments and return a QWidget
            3. If a QWidget it's used directly.
    :param script: A script to run after the widget is open.
            1. If a string it's a qualified name of a test function that can be run after the
            widget is created.
            2. If a callable it's used directly.

        The test function must have the signature:

            def test(widget):
                ...

        where argument widget is an instance of the tested widget.
        The test function can yield from time to time after which the widget can update itself.
        This will make the test non-blocking and changes can be viewed as the script runs.
        If the test yields an number it is interpreted as the number of seconds to wait
        until the next step.
        The test can yield a generator. In this case it will be iterated over until it stops
        and the iterations of the main script continue.
    :param attach_debugger: If true pause to let the user to attache a debugger before starting
        application.
    :param pause: A number of seconds to wait between the iterations.
    :param close_on_finish: An option to close the widget after the script finishes.
    :param is_cli: If true the script is to be run from a command line tool. Exceptions are
        treated slightly differently in this case.
    :param in_workbench: Set to True if the script will be run inside the workbench application.
    """
    if attach_debugger:
        input("Please attach the Debugger now if required. Press any key to continue")
    app = get_application()
    if widget_or_name is not None:
        widget_name = "Widget to test"
        if isinstance(widget_or_name, str):
            widget = create_widget(widget_or_name)
            widget_name = widget_or_name
        elif isinstance(widget_or_name, QWidget):
            widget = widget_or_name
        else:
            widget = widget_or_name()
        if hasattr(widget, "setWindowTitle"):
            widget.setWindowTitle(widget_name)
        if widget is not None:
            widget.show()
    else:
        widget = None

    script_runner = None
    if script is not None:
        try:
            script_runner = ScriptRunner(script, widget, close_on_finish=close_on_finish, is_cli=is_cli, pause=pause)
            script_runner.run()
        except Exception as e:
            if not is_cli:
                raise e

    if not in_workbench:
        ret = app.exec_()
        if not is_cli and script_runner is not None and script_runner.error is not None:
            raise script_runner.error
    else:
        ret = 0
    return ret


def run_script(script_or_name, widget):
    """
    Run a script passing the widget as an argument.
    :param script_or_name: A callable, class or a name.
    :param widget: A widget to interact with.
    :return: Output of the script or an Exception object.
    """
    if isinstance(script_or_name, str):
        module_name, fun_name = split_qualified_name(script_or_name)
        m = __import__(module_name, fromlist=[fun_name])
        fun = getattr(m, fun_name)
    else:
        fun = script_or_name
    try:
        if inspect.isclass(fun):
            fun = fun()
        return fun(widget)
    except Exception as e:
        traceback.print_exc()
        return e
