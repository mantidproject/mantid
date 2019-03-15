# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest

from mantidqt.widgets.messagedisplay import MessageDisplay
from mantidqt.utils.qt.testing import GuiTest


class MessageDisplayTest(GuiTest):
    """Minimal testing as it is exported from C++"""

    def test_widget_creation(self):
        display = MessageDisplay()
        self.assertTrue(display is not None)


if __name__ == "__main__":
    unittest.main()
