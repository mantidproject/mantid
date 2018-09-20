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
"""Helper class for testing modal widgets (dialogs).

Suggested usage:

class TestModalDialog(unittest.TestCase):

    def test_my_dialog(self):

        def testing_function(widget):
            widget.setTextValue('Hello World!')
            text = widget.textValue()
            self.assertEqual(text, 'Hello World!')
            widget.accept()

        tester = ModalTester(MyDialog, testing_function)
        tester.start()
        self.assertTrue(tester.passed)
"""
from __future__ import absolute_import

import traceback

from qtpy.QtCore import QTimer
from qtpy.QtWidgets import QApplication


class ModalTester(object):
    """
    Helper class for testing modal widgets (dialogs).
    """
    _qapp = None

    def __init__(self, creator, tester):
        """
        Initialise ModalTester.
        :param creator: Function without arguments that creates a modal widget. Can be a class name.
            A modal widget must have exec_() method.
        :param tester: A function taking a widget as its argument that does testing.
        """
        qapp = QApplication.instance()
        if qapp is None:
            self._qapp = QApplication([' '])
        else:
            self._qapp = qapp
        self.creator = creator
        self.tester = tester
        self.widget = None
        self.timer = None
        self.passed = False

    def __call__(self):
        """
        Initialise testing.
        """
        try:
            self.widget = self.creator()
        except:
            traceback.print_exc()
            self._qapp.exit(0)
        if self.widget is not None:
            self.widget.exec_()

    def _idle(self):
        """
        This function runs every time in QApplication's event loop.
        Call the testing function.
        """
        modal_widget = self._qapp.activeModalWidget()
        if modal_widget is not None:
            if self.widget is modal_widget:
                try:
                    self.tester(self.widget)
                    self.passed = True
                except:
                    traceback.print_exc()
                    self._qapp.exit(0)
            if modal_widget.isVisible():
                modal_widget.close()

    def start(self):
        """
        Start this tester.
        """
        self.timer = QTimer()
        # Connecting self.idle to a QTimer with 0 time delay
        # makes it called when there are no events to process
        self.timer.timeout.connect(self._idle)
        self.timer.start()
        # This calls __call__() method
        QTimer.singleShot(0, self)
        # Start the event loop
        self._qapp.exec_()
