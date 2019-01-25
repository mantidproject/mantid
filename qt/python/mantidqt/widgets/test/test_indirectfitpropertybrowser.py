from __future__ import (absolute_import, division, print_function)
import platform
import sys
import unittest

from qtpy import PYQT_VERSION
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import Qt, QMetaObject

from mantid import FrameworkManager
from mantidqt.utils.qt.test.gui_window_test import GuiWindowTest
from mantidqt.widgets.indirectfitpropertybrowser import IndirectFitPropertyBrowser


def on_ubuntu_or_darwin():
    return ('Ubuntu' in platform.platform() and sys.version[0] == '2' or
            sys.platform == 'darwin' and PYQT_VERSION[0] == '4')


@unittest.skipIf(on_ubuntu_or_darwin(), "Popups don't show on ubuntu with python 2. Unskip when switched to xvfb."
                                        "Qt4 has a bug on macs which is fixed in Qt5.")
class TestFitPropertyBrowser(GuiWindowTest):

    def create_widget(self):
        return IndirectFitPropertyBrowser()

    def test_find_peaks_no_workspace(self):
        print(self.widget.widget().layout())
        yield 10
        print('OK')


if __name__ == '__main__':
    unittest.main()
    FrameworkManager.clear()
