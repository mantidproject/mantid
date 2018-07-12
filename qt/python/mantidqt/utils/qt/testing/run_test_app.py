#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

import sys
import traceback

from qtpy.QtCore import QTimer
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.plugins import setup_library_paths


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
    raw_input('Please attach the Debugger now if required. Press any key to continue')
    setup_library_paths()
    app = QApplication([""])
    w = create_widget(widget_name)
    w.setWindowTitle(widget_name)
    w.show()

    if script is not None:
        try:
            # If script is a generator script_iter allows non-blocking
            # test execution
            script_iter = iter(run_script(script, w))
            pause_timer = QTimer()
            pause_timer.setSingleShot(True)

            def idle():
                if not pause_timer.isActive():
                    try:
                        # Run test script until the next 'yield'
                        pause_sec = script_iter.next()
                        if pause_sec is not None:
                            # Start non-blocking pause in seconds
                            pause_timer.start(int(pause_sec * 1000))
                    except StopIteration:
                        pass
                    except:
                        traceback.print_exc()
            timer = QTimer()
            # Zero-timeout timer runs idle() between Qt events
            timer.timeout.connect(idle)
            timer.start()
        except:
            pass

    sys.exit(app.exec_())


def run_script(script_name, widget):
    module_name, fun_name = split_qualified_name(script_name)
    m = __import__(module_name, fromlist=[fun_name])
    fun = getattr(m, fun_name)
    try:
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
    open_in_window(args.widget, args.script)
