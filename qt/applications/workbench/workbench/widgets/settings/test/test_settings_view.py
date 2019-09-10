# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

import unittest

from qtpy.QtWidgets import QApplication, QWidget

from mantid.py3compat.mock import MagicMock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from workbench.widgets.settings.presenter import SettingsPresenter


class MockWorkspaceWidget(QWidget):
    """
    This widget implements the ContextManager interface, when used
    in a `with MockWorkspaceWidget() as widget` statement it will
    mark delete itself on exiting the context.
    """

    def __init__(self):
        super(MockWorkspaceWidget, self).__init__(None)
        self.workspacewidget = MagicMock()

    def closeEvent(self, event):
        self.deleteLater()
        super(MockWorkspaceWidget, self).closeEvent(event)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        """
        On Exit triggers the closeEvent for the widget
        """
        self.close()


@start_qapplication
class SettingsViewTest(unittest.TestCase, QtWidgetFinder):
    def test_deletes_on_close(self):
        with MockWorkspaceWidget() as temp_widget:
            widget = SettingsPresenter(temp_widget)
            self.assert_widget_created()
            self.assert_widget_exists("GeneralSettings", expected_count=1)

            widget.view.close()

        QApplication.processEvents()

        self.assert_no_toplevel_widgets()
