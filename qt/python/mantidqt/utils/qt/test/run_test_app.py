# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""A small command line utility that can be used to manually test a widget in isolation.

Usage:

    python run_test_app.py qualified.name.of.SomeWidget [--script qualified.name.of.some_script]

The widget (and optional script) should be in the location form where python could import it as in:

    form qualified.name.of import SomeWidget

The widget is created by calling SomeWidget() constructor without arguments. If __init__ has arguments or
the widget needs to be specially prepared any function returning an instance of SomeWidget can be used instead.

Optional --script argument expects a name of a function which takes a widget as its single argument.
If specified the script is imported and run after the widget is created.

"""
from __future__ import absolute_import, print_function

import inspect
import six
import sys
import traceback

from qtpy.QtCore import QTimer
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.plugins import setup_library_paths

app = QApplication.instance()


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


class ScriptRunner(object):

    def __init__(self, script, w, close_on_finish):
        self.widget = w
        self.close_on_finish = close_on_finish
        ret = run_script(script, w)
        self.script_iter = iter(ret) if inspect.isgenerator(ret) else None
        self.parent_iter = None
        self.pause_timer = QTimer()
        self.pause_timer.setSingleShot(True)

    def __call__(self):
        global app
        if not self.pause_timer.isActive():
            try:
                if self.script_iter is None:
                    if self.close_on_finish:
                        self.widget.close()
                    return
                # Run test script until the next 'yield'
                pause_sec = self.script_iter.next()
                if pause_sec is not None:
                    if inspect.isgenerator(pause_sec):
                        self.parent_iter = self.script_iter
                        self.script_iter = pause_sec
                    else:
                        # Start non-blocking pause in seconds
                        self.pause_timer.start(int(pause_sec * 1000))
            except StopIteration:
                if self.parent_iter is not None:
                    self.script_iter = self.parent_iter
                    self.parent_iter = None
                elif self.close_on_finish:
                    self.widget.close()
            except:
                self.widget.close()
                traceback.print_exc()


def open_in_window(widget_name, script, attach_debugger=True, pause=0, close_on_finish=False):
    """
    Displays a widget in a window.
    :param widget_name:  A qualified name of a widget, ie mantidqt.mywidget.MyWidget
    :param script: A qualified name of a test function that can be run after the
        widget is created. The test function must have the signature:

            def test(widget):
                ...

        where argument widget is an instance of the tested widget.
        The test function can yield from time to time after which the widget can update itself.
        This will make the test non-blocking and changes can be viewed as the script runs.
        If the test yields an integer it is interpreted as the number of seconds to wait
        until the next step.
    """
    global app
    if attach_debugger:
        raw_input('Please attach the Debugger now if required. Press any key to continue')
    if app is None:
        setup_library_paths()
        app = QApplication([""])
    if isinstance(widget_name, six.string_types):
        w = create_widget(widget_name)
        w.setWindowTitle(widget_name)
    else:
        w = widget_name()
        w.setWindowTitle('Widget to test')
    w.show()

    if script is not None:
        try:
            idle = ScriptRunner(script, w, close_on_finish)
            timer = QTimer()
            if pause != 0:
                timer.setInterval(pause * 1000)
            # Zero-timeout timer runs idle() between Qt events
            timer.timeout.connect(idle)
            timer.start()
        except:
            pass

    return app.exec_()


def run_script(script_name, widget):
    if isinstance(script_name, six.string_types):
        module_name, fun_name = split_qualified_name(script_name)
        m = __import__(module_name, fromlist=[fun_name])
        fun = getattr(m, fun_name)
    else:
        fun = script_name
    try:
        if inspect.isclass(fun):
            fun = fun()
        return fun(widget)
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
    sys.exit(open_in_window(args.widget, args.script))
