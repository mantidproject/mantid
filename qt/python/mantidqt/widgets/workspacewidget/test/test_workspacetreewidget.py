# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import, print_function

import unittest

from mantidqt.widgets.workspacewidget.workspacetreewidget import WorkspaceTreeWidget
from mantidqt.utils.qt.testing import GuiTest
import sip


class WorkspaceWidgetTest(GuiTest):
    """Minimal testing as it is exported from C++"""

    def test_widget_creation(self):
        widget = WorkspaceTreeWidget()
        self.assertTrue(widget is not None)
        sip.delete(widget)


if __name__ == "__main__":
    unittest.main()
