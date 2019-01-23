# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


from __future__ import (absolute_import, division, print_function)

import unittest

from mock import Mock

from mantidqt.widgets.common.observing_view import ObservingView
from mantidqt.widgets.common.test_mocks.mock_qt import MockQtEvent, MockQtSignal


class MockObservingView(ObservingView):
    def __init__(self, _):
        ObservingView.__init__(self, _)
        self.close_signal = MockQtSignal()
        self.rename_signal = MockQtSignal()
        self.presenter = Mock(spec={'clear_observer'})
        self.setWindowTitle = Mock()


class ObservingViewTest(unittest.TestCase):

    def setUp(self):
        self.view = MockObservingView(None)

    def test_emit_close(self):
        self.view.emit_close()
        self.view.close_signal.emit.assert_called_once_with()

    def test_closeEvent(self):
        mock_event = MockQtEvent()
        self.view.closeEvent(mock_event)
        self.view.presenter.clear_observer.assert_called_once_with()
        mock_event.accept.assert_called_once_with()

    def test_rename(self):
        new_name = "123edsad"
        self.view.emit_rename(new_name)
        self.view.rename_signal.emit.assert_called_once_with(new_name)
        self.view.setWindowTitle.assert_called_once_with(self.view.TITLE_STRING.format(new_name))
