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
"""
Defines the QMainWindow of the application and the main() entry point.
"""
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

import sys

# -----------------------------------------------------------------------------
# Constants
# -----------------------------------------------------------------------------
ORIGINAL_SYS_EXIT = sys.exit
STDERR = sys.stderr

# -----------------------------------------------------------------------------
# Requirements
# -----------------------------------------------------------------------------
from workbench import requirements  # noqa
requirements.check_qt()

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------
from qtpy.QtCore import QCoreApplication, Qt  # noqa
from qtpy.QtWidgets import QApplication, QMainWindow  # noqa
from mantidqt.utils.qt import load_ui, plugins  # noqa

# Pre-application startup
plugins.setup_library_paths()
if hasattr(Qt, 'AA_EnableHighDpiScaling'):
    QCoreApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)


# -----------------------------------------------------------------------------
# Create the application instance early, set the application name for window
# titles and hold on to a reference to it. Required to be performed early so
# that the splash screen can be displayed
# -----------------------------------------------------------------------------

def qapplication():
    """Either return a reference to an existing application instance
    or create a new one
    :return: A reference to the QApplication object
    """
    app = QApplication.instance()
    if app is None:
        app = QApplication(['Mantid Workbench'])
    return app

# Create the application object early
MAIN_APP = qapplication()


# -----------------------------------------------------------------------------
# MainWindow
# -----------------------------------------------------------------------------
class MainWindow(QMainWindow):

    def __init__(self):
        QMainWindow.__init__(self)
        self._ui = load_ui(__file__, 'mainwindow.ui', baseinstance=self)
        MAIN_APP.setAttribute(Qt.AA_UseHighDpiPixmaps)


def initialize():
    """Perform an initialization of the application instance. Most notably
    this patches sys.exit so that it does nothing.

    :return: A reference to the existing application instance
    """
    app = qapplication()

    # Monkey patching sys.exit so users can't kill
    # the application this way
    def fake_sys_exit(arg=[]):
        pass
    sys.exit = fake_sys_exit

    return app


def start_workbench(app):
    """Given an application instance create the MainWindow,
    show it and start the main event loop
    """
    main_window = MainWindow()
    # do some pre-show setup...
    main_window.show()
    # do some pre-show setup...

    # lift-off!
    app.exec_()

    return main_window


def main():
    """Main entry point for the application"""

    # parse command arguments early

    # general initialization
    app = initialize()

    main_window = None
    try:
        main_window = start_workbench(app)
    except BaseException:
        # We count this as a crash
        import traceback
        # This is type of thing we want to capture and have reports
        # about. Prints to stderr as we can't really count on anything
        # else
        traceback.print_exc(file=STDERR)

    if main_window is None:
        # An exception occurred don't exit here
        return

    ORIGINAL_SYS_EXIT()


if __name__ == '__main__':
    main()
