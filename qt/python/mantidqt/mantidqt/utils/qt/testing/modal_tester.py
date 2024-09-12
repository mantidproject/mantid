# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
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
            self._qapp = QApplication([" "])
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
            if hasattr(self.widget, "exec_"):
                self.widget.exec_()
            else:
                self.widget.show()

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
