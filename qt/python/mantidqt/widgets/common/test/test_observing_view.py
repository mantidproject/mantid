# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, print_function)

import unittest

from mantidqt.widgets.common.test_mocks.mock_observing import MockObservingView
from mantidqt.widgets.common.test_mocks.mock_qt import MockQtEvent


class ObservingViewTest(unittest.TestCase):

    def setUp(self):
        self.view = MockObservingView(None)

    def test_emit_close(self):
        self.view.emit_close()
        self.view.close_signal.emit.assert_called_once_with()

    def test_closeEvent(self):
        mock_event = MockQtEvent()
        self.view.closeEvent(mock_event)
        mock_event.accept.assert_called_once_with()

    def test_rename(self):
        new_name = "123edsad"
        self.view.emit_rename(new_name)
        self.view.rename_signal.emit.assert_called_once_with(new_name)

    def test_rename_action(self):
        new_name = "123edsad"
        self.view._rename(new_name)
        self.view.setWindowTitle.assert_called_once_with(self.view.TITLE_STRING.format(new_name))
