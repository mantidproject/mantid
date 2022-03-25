# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
import unittest
from unittest import mock

from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.widgets.instrumentview.view import InstrumentView

from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class InstrumentViewManagerTest(unittest.TestCase):

    def test_presenter_close_called_correctly_on_view_close(self):
        """
        Check that the presenter's close function is called with the workspace name so that
        the manager can remove it correctly.
        """
        ws = CreateSampleWorkspace(OutputWorkspace="my_workspace")
        mock_presenter = mock.MagicMock()
        instrument_view = InstrumentView(parent=None, presenter=mock_presenter, name=ws.name())
        instrument_view.close()
        mock_presenter.close.assert_called_once_with(ws.name())


if __name__ == '__main__':
    unittest.main()
